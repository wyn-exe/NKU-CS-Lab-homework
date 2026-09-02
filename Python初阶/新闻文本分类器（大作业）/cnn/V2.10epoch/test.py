
#加载数据
import pandas as pd

def load_data(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    data = [line.strip().split('_separator_') for line in lines]
    return pd.DataFrame(data, columns=['id', 'content', 'keywords', 'category'])

def load_data2(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    data = [line.strip().split('_separator_') for line in lines]
    return pd.DataFrame(data, columns=['id', 'content', 'keywords'])

train_df = load_data('10epoch/train.txt')
test_df = load_data2('10epoch/test1.txt')


#文本预处理
import jieba

def preprocess_text(text):
    return ' '.join(jieba.cut(text))

train_df['content'] = train_df['content'].apply(preprocess_text)
test_df['content'] = test_df['content'].apply(preprocess_text)


#训练word2vec模型
from gensim.models import Word2Vec
# Combine all texts
texts = list(train_df['content']) + list(test_df['content'])
# Tokenize
tokens = [text.split() for text in texts]
# Train Word2Vec model
word2vec = Word2Vec(tokens, vector_size=100, window=5, min_count=1, workers=4)
word2vec.save("word2vec.model")


#构建TextCNN模型
import torch
import torch.nn as nn
import torch.nn.functional as F

class TextCNN(nn.Module):
    def __init__(self, vocab_size, embedding_dim, num_filters, filter_sizes, output_dim, dropout):
        super().__init__()
        self.embedding = nn.Embedding(vocab_size, embedding_dim)
        self.convs = nn.ModuleList([
            nn.Conv2d(in_channels=1, out_channels=num_filters, kernel_size=(fs, embedding_dim))
            for fs in filter_sizes
        ])
        self.fc = nn.Linear(num_filters * len(filter_sizes), output_dim)
        self.dropout = nn.Dropout(dropout)
        
    def forward(self, text):
        embedded = self.embedding(text).unsqueeze(1)
        conved = [F.relu(conv(embedded)).squeeze(3) for conv in self.convs]
        pooled = [F.max_pool1d(conv, conv.shape[2]).squeeze(2) for conv in conved]
        cat = self.dropout(torch.cat(pooled, dim=1))
        return self.fc(cat)

# Assume max_length is predefined
max_length = 100  # or the length you want
vocab_size = len(word2vec.wv.key_to_index) + 1  # Plus one for padding
embedding_dim = word2vec.vector_size
num_filters = 100
filter_sizes = [3, 4, 5]
output_dim = 15  # 0-14 categories
dropout = 0.5

model = TextCNN(vocab_size, embedding_dim, num_filters, filter_sizes, output_dim, dropout)


# 添加 '<PAD>' token 到 word2vec 的词汇表
word2vec.wv.add_vector('<PAD>', torch.zeros(word2vec.vector_size))
# 获取 '<PAD>' token 的索引
pad_index = word2vec.wv.key_to_index['<PAD>']



#准备数据
def pad_texts(texts, max_length):
    return [text + ['<PAD>'] * (max_length - len(text)) for text in texts]
# 准备数据，使用正确的 '<PAD>' 索引
X_train = pad_texts([text.split() for text in train_df['content']], max_length)
X_test = pad_texts([text.split() for text in test_df['content']], max_length)
# 转换为 PyTorch tensors
train_text = torch.tensor([[word2vec.wv.key_to_index.get(word, pad_index) for word in text] for text in X_train])
test_text = torch.tensor([[word2vec.wv.key_to_index.get(word, pad_index) for word in text] for text in X_test])
train_labels = torch.tensor(train_df['category'].astype(int))


#训练模型
import torch.optim as optim

# Define optimizer and loss
optimizer = optim.Adam(model.parameters())
criterion = nn.CrossEntropyLoss()

# Move model and data to GPU if available
model = model.to('cuda' if torch.cuda.is_available() else 'cpu')
train_text = train_text.to('cuda' if torch.cuda.is_available() else 'cpu')
train_labels = train_labels.to('cuda' if torch.cuda.is_available() else 'cpu')
train_labels = train_labels.long()  # 将标签转换为长整型

from torch.utils.data import DataLoader, TensorDataset
# 创建TensorDataset
train_dataset = TensorDataset(train_text, train_labels)
# 设置DataLoader的batch_size
batch_size = 16 
train_loader = DataLoader(train_dataset, batch_size=batch_size, shuffle=True)

# Train the model
num_epochs = 10
for epoch in range(num_epochs):
    model.train()
    for texts, labels in train_loader:
        optimizer.zero_grad()
        texts = texts.to('cuda' if torch.cuda.is_available() else 'cpu')
        labels = labels.to('cuda' if torch.cuda.is_available() else 'cpu')
        predictions = model(texts)
        loss = criterion(predictions.to('cuda' if torch.cuda.is_available() else 'cpu'), labels)
        loss.backward()
        optimizer.step()
    print(f'Epoch: {epoch+1}, Loss: {loss.item()}')

# 确保模型和数据都在GPU上
model = model.to('cuda' if torch.cuda.is_available() else 'cpu')
test_text = test_text.to('cuda' if torch.cuda.is_available() else 'cpu')


#评估模型并进行预测
model.eval()
with torch.no_grad():
    test_predictions = model(test_text)
    _, predicted = torch.max(test_predictions, 1)



#将预测结果保存到CSV文件
test_df['predicted_category'] = predicted.cpu().numpy()
test_df[['id', 'predicted_category']].to_csv('predictions.csv', index=False)
