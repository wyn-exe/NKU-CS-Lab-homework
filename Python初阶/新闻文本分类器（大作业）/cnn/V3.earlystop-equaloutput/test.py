import pandas as pd
import jieba
from gensim.models import Word2Vec
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
from sklearn.model_selection import train_test_split
from torch.utils.data import DataLoader, TensorDataset
import numpy as np

# 加载数据
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

train_df = load_data('earlystop/train.txt')
test_df = load_data2('earlystop/test1.txt')

# 文本预处理
def preprocess_text(text):
    return ' '.join(jieba.cut(text))

train_df['content'] = train_df['content'].apply(preprocess_text)
test_df['content'] = test_df['content'].apply(preprocess_text)

# 训练word2vec模型
texts = list(train_df['content']) + list(test_df['content'])
tokens = [text.split() for text in texts]
word2vec = Word2Vec(tokens, vector_size=100, window=5, min_count=1, workers=4)
word2vec.save("word2vec.model")

# 构建TextCNN模型
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

max_length = 100
vocab_size = len(word2vec.wv.key_to_index) + 1
embedding_dim = word2vec.vector_size
num_filters = 100
filter_sizes = [3, 4, 5]
output_dim = 15
dropout = 0.5

model = TextCNN(vocab_size, embedding_dim, num_filters, filter_sizes, output_dim, dropout)

# 添加 '<PAD>' token 到 word2vec 的词汇表
word2vec.wv.add_vector('<PAD>', torch.zeros(word2vec.vector_size))
pad_index = word2vec.wv.key_to_index['<PAD>']

# 准备数据
def pad_texts(texts, max_length):
    return [text + ['<PAD>'] * (max_length - len(text)) for text in texts]

X_train = pad_texts([text.split() for text in train_df['content']], max_length)
X_test = pad_texts([text.split() for text in test_df['content']], max_length)
train_text = torch.tensor([[word2vec.wv.key_to_index.get(word, pad_index) for word in text] for text in X_train])
test_text = torch.tensor([[word2vec.wv.key_to_index.get(word, pad_index) for word in text] for text in X_test])
train_labels = torch.tensor(train_df['category'].astype(int))

# 划分数据集
train_df, val_df = train_test_split(train_df, test_size=0.2, random_state=42)
X_val = pad_texts([text.split() for text in val_df['content']], max_length)
val_text = torch.tensor([[word2vec.wv.key_to_index.get(word, pad_index) for word in text] for text in X_val])
# 将Pandas Series转换为NumPy数组，然后转换为PyTorch tensor
val_labels = torch.tensor(val_df['category'].values.astype(int))

# 创建TensorDataset和DataLoader
train_dataset = TensorDataset(train_text, train_labels)
val_dataset = TensorDataset(val_text, val_labels)
batch_size = 16
train_loader = DataLoader(train_dataset, batch_size=batch_size, shuffle=True)
val_loader = DataLoader(val_dataset, batch_size=batch_size)

# 定义优化器和损失函数
optimizer = optim.Adam(model.parameters())
criterion = nn.CrossEntropyLoss()

# 将模型和数据移动到GPU（如果可用）
model = model.to('cuda' if torch.cuda.is_available() else 'cpu')
train_text = train_text.to('cuda' if torch.cuda.is_available() else 'cpu')
train_labels = train_labels.to('cuda' if torch.cuda.is_available() else 'cpu')
train_labels = train_labels.long()  # 确保标签是torch.long类型
val_text = val_text.to('cuda' if torch.cuda.is_available() else 'cpu')
val_labels = val_labels.to('cuda' if torch.cuda.is_available() else 'cpu')
val_labels = val_labels.long()  # 确保标签是torch.long类型

# 早停参数
patience = 3
best_val_loss = np.inf
patience_counter = 0

# 训练模型
num_epochs = 12
for epoch in range(num_epochs):
    model.train()
    train_loss = 0
    for texts, labels in train_loader:
        optimizer.zero_grad()
        texts = texts.to('cuda' if torch.cuda.is_available() else 'cpu')
        labels = labels.to('cuda' if torch.cuda.is_available() else 'cpu')
        labels = labels.long()  # 确保在每次迭代中标签都是torch.long类型
        predictions = model(texts)
        loss = criterion(predictions, labels)
        loss.backward()
        optimizer.step()
        train_loss += loss.item()
    
    # 验证模型
    model.eval()
    val_loss = 0
    with torch.no_grad():
        for texts, labels in val_loader:
            texts = texts.to('cuda' if torch.cuda.is_available() else 'cpu')
            labels = labels.to('cuda' if torch.cuda.is_available() else 'cpu')
            # 确保labels是torch.long类型
            labels = labels.long()
            predictions = model(texts)
            loss = criterion(predictions, labels)
            val_loss += loss.item()
    
    avg_train_loss = train_loss / len(train_loader)
    avg_val_loss = val_loss / len(val_loader)
    print(f'Epoch: {epoch+1}, Train Loss: {avg_train_loss}, Val Loss: {avg_val_loss}')
    
    # 早停逻辑
    if avg_val_loss < best_val_loss:
        best_val_loss = avg_val_loss
        patience_counter = 0
        # 保存最佳模型
        torch.save(model.state_dict(), 'best_model.pth')
    else:
        patience_counter += 1
        if patience_counter >= patience:
            print('Early stopping')
            break

# 加载最佳模型
model.load_state_dict(torch.load('best_model.pth'))

# 多次预测并保存最有可能的结果
num_predictions = 8  # 进行8次预测
predicted_categories = torch.zeros((len(test_text), num_predictions), dtype=torch.long)

# 进行多次预测
for i in range(num_predictions):
    model.eval()
    with torch.no_grad():
        test_predictions = model(test_text.to('cuda' if torch.cuda.is_available() else 'cpu'))
        _, predicted = torch.max(test_predictions, 1)
        predicted_categories[:, i] = predicted.cpu()

# 对预测结果进行投票
predicted_votes = torch.zeros((len(test_text), output_dim), dtype=torch.long)
for i in range(num_predictions):
    for j in range(len(test_text)):
        category = predicted_categories[j, i]
        predicted_votes[j, category] += 1

# 获取票数最多的类别
final_predictions = torch.argmax(predicted_votes, dim=1)

# 将预测结果保存到CSV文件
test_df['predicted_category'] = final_predictions.numpy()
test_df[['id', 'predicted_category']].to_csv('predictions.csv', index=False)
