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

# 训练word2vec模型
texts = list(train_df['content']) + list(test_df['content'])
tokens = [text.split() for text in texts]
word2vec = Word2Vec(tokens, vector_size=100, window=10, min_count=1, workers=4, epochs=10)
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
        
        # 初始化权重
        nn.init.xavier_uniform_(self.embedding.weight)  # 初始化嵌入层权重
        for conv in self.convs:
            nn.init.xavier_uniform_(conv.weight)  # 初始化卷积层权重
        nn.init.xavier_uniform_(self.fc.weight)  # 初始化全连接层权重
        nn.init.constant_(self.fc.bias, 0)  # 初始化全连接层偏置

        
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

# 过采样
def oversample_category(df, category, num_samples):
    # 获取样本
    category_df = df[df['category'] == category]
    
    # 随机选择num_samples个样本
    selected_samples = category_df.sample(n=num_samples, replace=True)
    
    # 返回过采样后的样本
    return selected_samples

# 过采样第14类的样本
oversampled_category_df = oversample_category(train_df, '14', 10000)  # 假设我们希望第14类的样本数量增加到4000
oversampled_category_df2 = oversample_category(train_df, '5', 10000)
# 合并过采样后的第14类样本
train_df = pd.concat([train_df, oversampled_category_df])
train_df = pd.concat([train_df, oversampled_category_df2])

# 准备数据
def pad_texts(texts, max_length):
    return [text + ['<PAD>'] * (max_length - len(text)) for text in texts]

X_train = pad_texts([text.split() for text in train_df['content']], max_length)
X_test = pad_texts([text.split() for text in test_df['content']], max_length)
train_text = torch.tensor([[word2vec.wv.key_to_index.get(word, pad_index) for word in text] for text in X_train])
test_text = torch.tensor([[word2vec.wv.key_to_index.get(word, pad_index) for word in text] for text in X_test])
try:
    train_df['category'] = train_df['category'].astype(int)
except ValueError as e:
    print("ValueError:", e)
    # 你可以进一步检查哪些值无法转换为整数
    non_numeric_categories = train_df['category'][train_df['category'].apply(lambda x: not isinstance(x, int))]
    print(non_numeric_categories)
# 再次尝试创建 PyTorch 张量
train_labels = torch.tensor(train_df['category'].values.astype(int))

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
optimizer = optim.SGD(model.parameters(), lr=0.01, momentum=0.9)  # 设置学习率为0.01，动量为0.9
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
best_val_accuracy = 0.0
patience_counter = 0
loss_threshold = 0.01  # 当损失低于这个值时，开始更加关注精度

# 训练模型
num_epochs = 20
for epoch in range(num_epochs):
    model.train()
    train_loss = 0
    correct_predictions = {}  # 用于存储每个类别的正确预测数量
    total_predictions = {}  # 用于存储每个类别的总预测数量

    for texts, labels in train_loader:
        optimizer.zero_grad()  # 清空之前的梯度
        texts = texts.to('cuda' if torch.cuda.is_available() else 'cpu')
        labels = labels.to('cuda' if torch.cuda.is_available() else 'cpu')
        labels = labels.long()  # 确保在每次迭代中标签都是torch.long类型
        predictions = model(texts)
        loss = criterion(predictions, labels)
        loss.backward()  # 反向传播
        optimizer.step()  # 应用梯度更新权重
        train_loss += loss.item()

        # 计算每个类别的正确预测数量和总预测数量
        _, predicted = torch.max(predictions, 1)
        for i in range(labels.size(0)):
            label = labels[i].item()
            predicted_label = predicted[i].item()
            if label not in correct_predictions:
                correct_predictions[label] = 0
                total_predictions[label] = 0
            if predicted_label == label:
                correct_predictions[label] += 1
            total_predictions[label] += 1

    # 验证模型
    model.eval()
    val_loss = 0
    val_accuracy = 0
    total = 0
    with torch.no_grad():
        for texts, labels in val_loader:
            texts = texts.to('cuda' if torch.cuda.is_available() else 'cpu')
            labels = labels.to('cuda' if torch.cuda.is_available() else 'cpu')
            labels = labels.long()  # 确保在每次迭代中标签都是torch.long类型
            predictions = model(texts)
            loss = criterion(predictions, labels)
            val_loss += loss.item()

            # 计算精度
            _, predicted = torch.max(predictions, 1)
            accuracy = (predicted == labels).sum().item() / labels.size(0)
            val_accuracy += accuracy
            total += labels.size(0)

            # 计算每个类别的正确预测数量和总预测数量
            _, predicted = torch.max(predictions, 1)
            for i in range(labels.size(0)):
                label = labels[i].item()
                predicted_label = predicted[i].item()
                if label not in correct_predictions:
                    correct_predictions[label] = 0
                    total_predictions[label] = 0
                if predicted_label == label:
                    correct_predictions[label] += 1
                total_predictions[label] += 1

    avg_train_loss = train_loss / len(train_loader)
    avg_val_loss = val_loss / len(val_loader)
    avg_val_accuracy = val_accuracy / len(val_loader)
    print(f'Epoch: {epoch+1}, Train Loss: {avg_train_loss}, Val Loss: {avg_val_loss}, Val Accuracy: {avg_val_accuracy:.4f}')

    # 计算每个类别的准确率
    for label in correct_predictions:
        print(f'Epoch {epoch+1}: Category {label} - Correct Predictions: {correct_predictions[label]}, Total Predictions: {total_predictions[label]}, Accuracy: {(correct_predictions[label] / total_predictions[label]) * 100:.2f}%')

    # 早停逻辑
    if avg_val_loss < best_val_loss and avg_val_loss >= loss_threshold:
        best_val_loss = avg_val_loss
        patience_counter = 0
        # 保存最佳模型
        torch.save(model.state_dict(), 'best_model.pth')
        print(f'Best model saved at epoch {epoch+1} with val loss {best_val_loss:.4f}')
    elif avg_val_loss < loss_threshold and avg_val_accuracy > best_val_accuracy:
        best_val_accuracy = avg_val_accuracy
        patience_counter = 0
        # 保存最佳模型
        torch.save(model.state_dict(), 'best_model.pth')
        print(f'Best model saved at epoch {epoch+1} with val accuracy {best_val_accuracy:.4f}')
    else:
        patience_counter += 1
        if patience_counter >= patience:
            print(f'Early stopping at epoch {epoch+1} with val loss {avg_val_loss:.4f} and val accuracy {avg_val_accuracy:.4f}')
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
