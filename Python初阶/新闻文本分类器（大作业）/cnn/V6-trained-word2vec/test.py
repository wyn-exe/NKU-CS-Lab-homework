import pandas as pd
import jieba
from gensim.models import KeyedVectors
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
from torch.optim.lr_scheduler import ReduceLROnPlateau
from sklearn.model_selection import train_test_split
from torch.utils.data import DataLoader, TensorDataset
import numpy as np
import gensim

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

train_df = load_data('train.txt')
test_df = load_data2('test1.txt')

# 文本预处理
# 读取停用词表
stopwords_path = '百度停用词表.txt'
with open(stopwords_path, 'r', encoding='utf-8') as f:
    stopwords = set([line.strip() for line in f.readlines()])
# 去除停用词
def preprocess_text(text):
    words = jieba.cut(text)
    filtered_words = [word for word in words if word not in stopwords]
    return ' '.join(filtered_words)
# 处理文本数据
train_df['content'] = train_df['content'].apply(preprocess_text)
test_df['content'] = test_df['content'].apply(preprocess_text)

# 过采样
def oversample_category(df, category, num_samples):
    # 获取样本
    category_df = df[df['category'] == category]
    
    # 随机选择num_samples个样本
    selected_samples = category_df.sample(n=num_samples, replace=True)
    
    # 返回过采样后的样本
    return selected_samples

# 过采样
oversampled_category_df = oversample_category(train_df, '14', 9000) 
oversampled_category_df2 = oversample_category(train_df, '5', 9000) 
# 合并过采样后的样本
train_df = pd.concat([train_df, oversampled_category_df])
train_df = pd.concat([train_df, oversampled_category_df2])

# 加载预训练的Word2Vec模型
pretrained_word2vec_path = 'cnn/trained-word2vec/baike_26g_news_13g_novel_229g.model'
word_vectors = KeyedVectors.load(pretrained_word2vec_path)

# 创建训练集和测试集的词表
train_texts = list(train_df['content'])
test_texts = list(test_df['content'])
all_texts = train_texts + test_texts
all_tokens = [jieba.cut(text) for text in all_texts]
train_tokens = [jieba.cut(text) for text in train_texts]
test_tokens = [jieba.cut(text) for text in test_texts]

train_vocab = set(word for text in train_tokens for word in text)
test_vocab = set(word for text in test_tokens for word in text)
all_vocab = train_vocab.union(test_vocab)

# 初始化词向量矩阵
embedding_dim = word_vectors.vector_size
vocab_size = len(all_vocab) + 2  # 加2是为了包括<PAD>和<UNK>
embedding_matrix = np.zeros((vocab_size, embedding_dim))

# 填充词向量矩阵
word_to_idx = {'<PAD>': 0, '<UNK>': 1}  # 初始化PAD和UNK的索引
for word in all_vocab:
    if word in word_vectors.wv.key_to_index:  # 使用正确的属性
        word_idx = len(word_to_idx)
        word_to_idx[word] = word_idx
        embedding_matrix[word_idx] = word_vectors.wv[word]  # 使用.wv来访问单词向量

# 未登录词向量初始化
unk_vector = np.random.rand(embedding_dim)
unk_idx = len(word_to_idx)
word_to_idx['<UNK>'] = unk_idx
embedding_matrix[unk_idx] = unk_vector

# 现在设置vocab_size为embedding_matrix的行数
vocab_size = embedding_matrix.shape[0]

# 构建TextCNN模型
class TextCNN(nn.Module):
    def __init__(self, vocab_size, embedding_dim, num_filters, filter_sizes, output_dim, dropout):
        super().__init__()
        self.embedding = nn.Embedding(vocab_size, embedding_dim)
        self.convs = nn.ModuleList([
            nn.Conv2d(in_channels=1, out_channels=num_filters, kernel_size=(fs, embedding_dim))
            for fs in filter_sizes
        ])
        self.bns = nn.ModuleList([
            nn.BatchNorm2d(num_filters)  # 添加BN层
            for _ in filter_sizes
        ])
        self.fc = nn.Linear(num_filters * len(filter_sizes), output_dim)
        self.dropout = nn.Dropout(dropout)
        
        # 初始化权重
        nn.init.xavier_uniform_(self.embedding.weight)  # 初始化嵌入层权重
        for conv in self.convs:
            nn.init.xavier_uniform_(conv.weight)  # 初始化卷积层权重
            nn.init.constant_(conv.bias, 0.01)  # 初始化偏置为0.01
        nn.init.xavier_uniform_(self.fc.weight)  # 初始化全连接层权重
        nn.init.constant_(self.fc.bias, 0.01)  # 初始化全连接层偏置

        
    def forward(self, text):
        embedded = self.embedding(text).unsqueeze(1)
        conved = []
        for conv, bn in zip(self.convs, self.bns):
            c = conv(embedded)
            c = bn(c)  # 应用BN层
            c = F.relu(c)
            conved.append(c.squeeze(3))
        pooled = [F.max_pool1d(conv, conv.shape[2]).squeeze(2) for conv in conved]
        cat = self.dropout(torch.cat(pooled, dim=1))
        return self.fc(cat)

max_length = 100
embedding_dim = word_vectors.vector_size
num_filters = 100
filter_sizes = [3, 4, 5]
output_dim = 15
dropout = 0.3

model = TextCNN(vocab_size, embedding_dim, num_filters, filter_sizes, output_dim, dropout)
model.embedding.weight.data.copy_(torch.from_numpy(embedding_matrix))
model.embedding.weight.requires_grad = False  # 冻结嵌入层权重

# 添加 '<PAD>' token 到 word2vec 的词汇表
word_vectors.wv.add_vector('<PAD>', torch.zeros(word_vectors.vector_size))
pad_index = word_vectors.wv.key_to_index['<PAD>']

# 定义准备数据的函数
def prepare_data(texts, word_to_idx, max_length):
    padded_texts = []
    for text in texts:
        # 将文本分词
        tokens = list(jieba.cut(text))
        # 将单词转换为索引
        indexed_tokens = [word_to_idx.get(word, word_to_idx['<UNK>']) for word in tokens]
        # 填充或截断序列
        if len(indexed_tokens) > max_length:
            indexed_tokens = indexed_tokens[:max_length]
        else:
            indexed_tokens += [word_to_idx['<PAD>']] * (max_length - len(indexed_tokens))
        padded_texts.append(indexed_tokens)
    return np.array(padded_texts)

# 准备训练数据
X_train = prepare_data(train_texts, word_to_idx, max_length)
train_text = torch.tensor(X_train)
# 准备测试数据
X_test = prepare_data(test_texts, word_to_idx, max_length)
test_text = torch.tensor(X_test)

try:
    train_df['category'] = train_df['category'].astype(int)
except ValueError as e:
    print("ValueError:", e)
    # 你可以进一步检查哪些值无法转换为整数
    non_numeric_categories = train_df['category'][train_df['category'].apply(lambda x: not isinstance(x, int))]
    print(non_numeric_categories)
# 尝试创建 PyTorch 张量
train_labels = torch.tensor(train_df['category'].values.astype(int))

# 划分数据集
X_train, X_val, y_train, y_val = train_test_split(X_train, train_labels, test_size=0.2, random_state=42)
y_train = torch.tensor(y_train)
y_val = torch.tensor(y_val)

# 创建TensorDataset和DataLoader
train_dataset = TensorDataset(torch.tensor(X_train), y_train)
val_dataset = TensorDataset(torch.tensor(X_val), y_val)
batch_size = 32
train_loader = DataLoader(train_dataset, batch_size=batch_size, shuffle=True)
val_loader = DataLoader(val_dataset, batch_size=batch_size, shuffle=False)

# 定义优化器和损失函数
optimizer = optim.SGD(model.parameters(), lr=0.01, momentum=0.9)
criterion = nn.CrossEntropyLoss()
scheduler = ReduceLROnPlateau(optimizer, mode='min', factor=0.5, patience=3, verbose=True)

# 将模型移动到GPU（如果可用）
model = model.to('cuda' if torch.cuda.is_available() else 'cpu')

# 早停参数
patience = 10
best_val_loss = float('inf')
patience_counter = 0

# 训练模型
num_epochs = 100
for epoch in range(num_epochs):
    model.train()
    train_loss = 0
    val_loss = 0
    train_correct = 0
    val_correct = 0

    # 训练阶段
    for texts, labels in train_loader:
        texts = texts.to('cuda' if torch.cuda.is_available() else 'cpu')
        labels = labels.to('cuda' if torch.cuda.is_available() else 'cpu')
        labels = labels.long()
        predictions = model(texts)
        loss = criterion(predictions, labels)
        optimizer.zero_grad()
        loss.backward()
        optimizer.step()
        train_loss += loss.item()
        train_correct += (predictions.argmax(1) == labels).sum().item()

    # 验证阶段
    model.eval()
    with torch.no_grad():
        for texts, labels in val_loader:
            texts = texts.to('cuda' if torch.cuda.is_available() else 'cpu')
            labels = labels.to('cuda' if torch.cuda.is_available() else 'cpu')
            labels = labels.long()
            predictions = model(texts)
            loss = criterion(predictions, labels)
            val_loss += loss.item()
            val_correct += (predictions.argmax(1) == labels).sum().item()

    train_loss /= len(train_loader)
    val_loss /= len(val_loader)
    train_accuracy = train_correct / len(train_dataset)
    val_accuracy = val_correct / len(val_dataset)

    print(f'Epoch: {epoch+1}, Train Loss: {train_loss:.4f}, Val Loss: {val_loss:.4f}, '
          f'Train Accuracy: {train_accuracy:.4f}, Val Accuracy: {val_accuracy:.4f}')
    
    # 更新学习率
    scheduler.step(val_loss)

    # 早停逻辑
    if val_loss < best_val_loss:
        best_val_loss = val_loss
        patience_counter = 0
        torch.save(model.state_dict(), 'best_model.pth')
    else:
        patience_counter += 1
        if patience_counter >= patience:
            print('Early stopping')
            break


# 加载最佳模型
model.load_state_dict(torch.load('best_model.pth'))

#评估模型并进行预测
model.eval()
with torch.no_grad():
    test_predictions = model(test_text.to('cuda'))  # 将测试集张量移动到GPU
    _, predicted = torch.max(test_predictions, 1)



#将预测结果保存到CSV文件
test_df['predicted_category'] = predicted.cpu().numpy()
test_df[['id', 'predicted_category']].to_csv('predictions.csv', index=False)
