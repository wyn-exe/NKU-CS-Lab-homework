
#文本预处理
import pandas as pd
import jieba
import numpy as np
import matplotlib.pyplot as plt
plt.rcParams['font.sans-serif'] = ['KaiTi']  #指定默认字体 SimHei黑体
plt.rcParams['axes.unicode_minus'] = False   #解决保存图像是负号'
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
    # 去停用词
stop_list  = pd.read_csv("百度停用词表.txt",index_col=False,quoting=3,
                         sep="\t",names=['stopword'], encoding='utf-8')
    #Jieba分词函数
def txt_cut(juzi):
    lis=[w for w in jieba.lcut(juzi) if w not in stop_list.values]
    return " ".join(lis)
train_df['content'] = train_df['content'].astype('str').apply(txt_cut)
test_df['content'] = test_df['content'].astype('str').apply(txt_cut)
train_df['content'] = train_df['content']+(train_df['keywords'].astype('str').apply(txt_cut))
test_df['content'] = test_df['content']+(test_df['keywords'].astype('str').apply(txt_cut))
train_df['category'].value_counts().plot(kind='bar')
plt.show()   
    # 过采样
def oversample_category(df, category, num_samples):
    # 获取样本
    category_df = df[df['category'] == category]
    
    # 随机选择num_samples个样本
    selected_samples = category_df.sample(n=num_samples, replace=True)
    
    # 返回过采样后的样本
    return selected_samples
oversampled_category_df3 = oversample_category(train_df, '3', 2000) 
oversampled_category_df4 = oversample_category(train_df, '4', 2000) 
oversampled_category_df5 = oversample_category(train_df, '5', 8000) 
oversampled_category_df7 = oversample_category(train_df, '7', 2000) 
oversampled_category_df8 = oversample_category(train_df, '8', 2000) 
oversampled_category_df9 = oversample_category(train_df, '9', 2000)  
oversampled_category_df10 = oversample_category(train_df, '10', 2000) 
oversampled_category_df12 = oversample_category(train_df, '12', 2000) 
oversampled_category_df13 = oversample_category(train_df, '13', 2000) 
oversampled_category_df14 = oversample_category(train_df, '14', 10000)  
    # 合并过采样后的样本
train_df = pd.concat([train_df, oversampled_category_df3])
train_df = pd.concat([train_df, oversampled_category_df4])
train_df = pd.concat([train_df, oversampled_category_df5])
train_df = pd.concat([train_df, oversampled_category_df7])
train_df = pd.concat([train_df, oversampled_category_df8])
train_df = pd.concat([train_df, oversampled_category_df9])
train_df = pd.concat([train_df, oversampled_category_df10])
train_df = pd.concat([train_df, oversampled_category_df12])
train_df = pd.concat([train_df, oversampled_category_df13])
train_df = pd.concat([train_df, oversampled_category_df14])
train_df =train_df.sample(frac=1)
train_df['category'].value_counts().plot(kind='bar')
plt.show()   
train_df['category'] = train_df['category'].astype(int)
#文本向量化
from os import listdir
from tensorflow import keras
from sklearn.model_selection import train_test_split
# 将文件分割, 建立词索引字典     
tok = keras.preprocessing.text.Tokenizer(num_words=140000)
tok.fit_on_texts(train_df['content'].values)
print({k: tok.word_index[k] for k in list(tok.word_index)[:10]})
X= tok.texts_to_sequences(train_df['content'].values)
#查看x的长度的分布
length=[]
for i in X:
    length.append(len(i))
v_c=pd.Series(length).value_counts()
print(v_c[v_c>150])
v_c[v_c>150].plot(kind='bar',figsize=(12,5))
plt.show()
X= keras.preprocessing.sequence.pad_sequences(X, maxlen=40)
Y=train_df['category'].values

#划分验证集和训练集
X_train, X_test, Y_train, Y_test =  train_test_split(X, Y, test_size=0.2, stratify=Y, random_state=42)
Y_test_original=Y_test.copy()
Y_train = keras.utils.to_categorical(Y_train)
Y_test = keras.utils.to_categorical(Y_test)
Y= keras.utils.to_categorical(Y)



#构建神经网络
    #构建Transformer模块
import tensorflow as tf
class TransformerEncoder(keras.layers.Layer):
    def __init__(self, embed_dim, dense_dim, num_heads, **kwargs):
        super().__init__(**kwargs)
        self.embed_dim = embed_dim
        self.dense_dim = dense_dim
        self.num_heads = num_heads
        self.attention = keras.layers.MultiHeadAttention(num_heads=num_heads, key_dim=embed_dim)
        self.dense_proj = keras.Sequential(
            [keras.layers.Dense(dense_dim, activation="relu"),keras.layers.Dense(embed_dim),] )
        self.layernorm_1 = keras.layers.LayerNormalization()
        self.layernorm_2 = keras.layers.LayerNormalization()
 
    def call(self, inputs, mask=None):
        if mask is not None:
            mask = mask[:, tf.newaxis, :]
        attention_output = self.attention(inputs, inputs, attention_mask=mask)
        proj_input = self.layernorm_1(inputs + attention_output)
        proj_output = self.dense_proj(proj_input)
        return self.layernorm_2(proj_input + proj_output)
 
    def get_config(self):
        config = super().get_config()
        config.update({
            "embed_dim": self.embed_dim,
            "num_heads": self.num_heads,
            "dense_dim": self.dense_dim, })
        return config
  #Transformer还需要位置编码
class PositionalEmbedding(keras.layers.Layer):
    def __init__(self, sequence_length, input_dim, output_dim, **kwargs):
        super().__init__(**kwargs)
        self.token_embeddings = keras.layers.Embedding(input_dim=input_dim, output_dim=output_dim)
        self.position_embeddings = keras.layers.Embedding(input_dim=sequence_length, output_dim=output_dim)
        self.sequence_length = sequence_length
        self.input_dim = input_dim
        self.output_dim = output_dim
 
    def call(self, inputs):
        length = tf.shape(inputs)[-1]
        positions = tf.range(start=0, limit=length, delta=1)
        embedded_tokens = self.token_embeddings(inputs)
        embedded_positions = self.position_embeddings(positions)
        return embedded_tokens + embedded_positions
 
    def compute_mask(self, inputs, mask=None):
        class MyLayer(keras.layers.Layer):
            def call(self, inputs):
                output = tf.math.not_equal(inputs, 0)
                return output
        layer = MyLayer()
        return layer(inputs)
 
    def get_config(self):
        config = super().get_config()
        config.update({
            "output_dim": self.output_dim,
            "sequence_length": self.sequence_length,
            "input_dim": self.input_dim,})
        return config
  #设定参数
np.random.seed(0)  # 指定随机数种子  
#单词索引的最大个数40000，单句话最大长度40
top_words=140000  
max_words=40    #序列长度
embed_dim=128    #嵌入维度
num_labels=15   #15分类

#构建可用模型
def build_model(top_words=top_words,max_words=max_words,num_labels=num_labels,mode='LSTM',hidden_dim=[64]):
    if mode=='RNN':
        model = keras.models.Sequential()
        model.add(keras.layers.Embedding(top_words, input_length=max_words, output_dim=embed_dim, mask_zero=True))
        model.add(keras.layers.Dropout(0.5))
        model.add(keras.layers.SimpleRNN(hidden_dim[0]))  
        model.add(keras.layers.Dropout(0.5))  
        model.add(keras.layers.Dense(num_labels, activation="softmax"))
    elif mode=='MLP':
        model = keras.models.Sequential()
        model.add(keras.layers.Embedding(top_words, input_length=max_words, output_dim=embed_dim))#, mask_zero=True
        model.add(keras.layers.Flatten())
        model.add(keras.layers.Dropout(0.5))
        model.add(keras.layers.Dense(hidden_dim[0]))  
        model.add(keras.layers.Dropout(0.5))   
        model.add(keras.layers.Dense(num_labels, activation="softmax"))
    elif mode=='LSTM':
        model = keras.models.Sequential()
        model.add(keras.layers.Embedding(top_words, input_length=max_words, output_dim=embed_dim))
        model.add(keras.layers.Dropout(0.5))
        model.add(keras.layers.LSTM(hidden_dim[0]))
        model.add(keras.layers.Dropout(0.5))   
        model.add(keras.layers.Dense(num_labels, activation="softmax"))
    elif mode=='GRU':
        model = keras.models.Sequential()
        model.add(keras.layers.Embedding(top_words, input_length=max_words, output_dim=embed_dim))
        model.add(keras.layers.Dropout(0.5))
        model.add(keras.layers.GRU(hidden_dim[0]))
        model.add(keras.layers.Dropout(0.5))   
        model.add(keras.layers.Dense(num_labels, activation="softmax"))
    elif mode=='CNN':        #一维卷积
        model = keras.models.Sequential()
        model.add(keras.layers.Embedding(top_words, input_length=max_words, output_dim=embed_dim, mask_zero=True))
        model.add(keras.layers.Dropout(0.5))
        model.add(keras.layers.Conv1D(filters=32, kernel_size=3, padding="same",activation="relu"))
        model.add(keras.layers.MaxPooling1D(pool_size=2))
        model.add(keras.layers.Flatten())
        model.add(keras.layers.Dense(hidden_dim[0], activation="relu"))
        model.add(keras.layers.Dropout(0.5))   
        model.add(keras.layers.Dense(num_labels, activation="softmax"))
    elif mode=='CNN+LSTM':
        model = keras.models.Sequential()
        model.add(keras.layers.Embedding(top_words, input_length=max_words, output_dim=embed_dim))
        model.add(keras.layers.Dropout(0.5))    
        model.add(keras.layers.Conv1D(filters=32, kernel_size=3, padding="same",activation="relu"))
        model.add(keras.layers.MaxPooling1D(pool_size=2))
        model.add(keras.layers.LSTM(hidden_dim[0]))
        model.add(keras.layers.Dropout(0.5))   
        model.add(keras.layers.Dense(num_labels, activation="softmax"))
    elif mode=='BiLSTM':
        model = keras.models.Sequential()
        model.add(keras.layers.Embedding(top_words, input_length=max_words, output_dim=embed_dim))
        model.add(keras.layers.Bidirectional(keras.layers.LSTM(64)))
        model.add(keras.layers.Dense(hidden_dim[0], activation='relu'))
        model.add(keras.layers.Dropout(0.5))
        model.add(keras.layers.Dense(num_labels, activation='softmax'))
    #下面的网络采用Funcional API实现
    elif mode=='TextCNN':
        inputs = keras.layers.Input(name='inputs',shape=[max_words,], dtype='float64')
        ## 词嵌入使用预训练的词向量
        layer = keras.layers.Embedding(top_words, input_length=max_words, output_dim=embed_dim)(inputs)
        ## 词窗大小分别为3,4,5
        cnn1 = keras.layers.Conv1D(32, 2, padding='same', strides = 1, activation='relu')(layer)
        cnn1 = keras.layers.MaxPooling1D(pool_size=2)(cnn1)
        cnn2 = keras.layers.Conv1D(32, 3, padding='same', strides = 1, activation='relu')(layer)
        cnn2 = keras.layers.MaxPooling1D(pool_size=2)(cnn2)
        cnn3 = keras.layers.Conv1D(32, 4, padding='same', strides = 1, activation='relu')(layer)
        cnn3 = keras.layers.MaxPooling1D(pool_size=2)(cnn3)
        # 合并三个模型的输出向量
        cnn = keras.layers.Concatenate(axis=-1)([cnn1, cnn2, cnn3])
        x = keras.layers.Flatten()(cnn) 
        x = keras.layers.Dense(hidden_dim[0], activation='relu')(x)
        output = keras.layers.Dense(num_labels, activation='softmax')(x)
        model = keras.models.Model(inputs=inputs, outputs=output)           
    elif mode=='Transformer':
        inputs = keras.layers.Input(name='inputs',shape=[max_words,], dtype='float64')
        x = keras.layers.Embedding(top_words, input_length=max_words, output_dim=embed_dim, mask_zero=True)(inputs)
        x = TransformerEncoder(embed_dim, 32, 4)(x)
        x = keras.layers.GlobalMaxPooling1D()(x)
        x = keras.layers.Dropout(0.5)(x)
        outputs = keras.layers.Dense(num_labels, activation='softmax')(x)
        model = keras.models.Model(inputs, outputs)           
    elif mode=='PositionalEmbedding+Transformer':
        inputs = keras.layers.Input(name='inputs',shape=[max_words,], dtype='float64')
        x= PositionalEmbedding(sequence_length=max_words, input_dim=top_words, output_dim=embed_dim)(inputs)
        x = TransformerEncoder(embed_dim, 32, 4)(x)
        x = keras.layers.GlobalMaxPooling1D()(x)
        x = keras.layers.Dropout(0.5)(x)
        outputs = keras.layers.Dense(num_labels, activation='softmax')(x)
        model = keras.models.Model(inputs, outputs)
        
    model.compile(loss="categorical_crossentropy", optimizer="adam", metrics=["accuracy"])
    return model


#定义损失和精度的图,和混淆矩阵指标等等
from sklearn.metrics import confusion_matrix
from sklearn.metrics import classification_report
from sklearn.metrics import cohen_kappa_score
import seaborn as sns
def plot_loss(history):
    # 显示训练和验证损失图表
    plt.subplots(1,2,figsize=(10,3))
    plt.subplot(121)
    loss = history.history["loss"]
    epochs = range(1, len(loss)+1)
    val_loss = history.history["val_loss"]
    plt.plot(epochs, loss, "bo", label="Training Loss")
    plt.plot(epochs, val_loss, "r", label="Validation Loss")
    plt.title("Training and Validation Loss")
    plt.xlabel("Epochs")
    plt.ylabel("Loss")
    plt.legend()  
    plt.subplot(122)
    acc = history.history["accuracy"]
    val_acc = history.history["val_accuracy"]
    plt.plot(epochs, acc, "b-", label="Training Acc")
    plt.plot(epochs, val_acc, "r--", label="Validation Acc")
    plt.title("Training and Validation Accuracy")
    plt.xlabel("Epochs")
    plt.ylabel("Accuracy")
    plt.legend()
    plt.tight_layout()
    plt.show()
def plot_confusion_matrix(model,X_test,Y_test_original):
    #预测概率
    prob=model.predict(X_test) 
    #预测类别
    pred=np.argmax(prob,axis=1)
    #数据透视表，混淆矩阵
    pred=pd.Series(pred)
    Y_test_original=pd.Series(Y_test_original)
    table = pd.crosstab(Y_test_original, pred, rownames=['Actual'], colnames=['Predicted'])
    #print(table)
    sns.heatmap(table,cmap='Blues',fmt='.20g', annot=True)
    plt.tight_layout()
    plt.show()
    #计算混淆矩阵的各项指标
    print(classification_report(Y_test_original, pred))
    #科恩Kappa指标
    print('科恩Kappa'+str(cohen_kappa_score(Y_test_original, pred)))
 
def evaluation(y_test, y_predict):
    accuracy=classification_report(y_test, y_predict,output_dict=True)['accuracy']
    s=classification_report(y_test, y_predict,output_dict=True)['weighted avg']
    precision=s['precision']
    recall=s['recall']
    f1_score=s['f1-score']
    #kappa=cohen_kappa_score(y_test, y_predict)
    return accuracy,precision,recall,f1_score #, kappa

import pickle
#定义训练函数
df_eval=pd.DataFrame(columns=['Accuracy','Precision','Recall','F1_score'])
def train_fuc(max_words=max_words,mode='BiLSTM+Attention',batch_size=64,epochs=10,hidden_dim=[64],show_loss=True,show_confusion_matrix=True):
    #构建模型
    model=build_model(max_words=max_words,mode=mode,hidden_dim=hidden_dim)
    print(model.summary())
    es = keras.callbacks.EarlyStopping(patience=1)
    history=model.fit(X_train, Y_train,batch_size=batch_size,epochs=epochs,validation_split=0.2, verbose=1,callbacks=[es])
    print('——————————-----------------——训练完毕—————-----------------------------———————')
    
    # 评估模型
    loss, accuracy = model.evaluate(X_test, Y_test)  ;  print("测试数据集的准确度 = {:.4f}".format(accuracy))
    prob=model.predict(X_test) ;  pred=np.argmax(prob,axis=1)
    score=list(evaluation(Y_test_original, pred))
    df_eval.loc[mode,:]=score
    
    if show_loss:
        plot_loss(history)
    if show_confusion_matrix:
        plot_confusion_matrix(model=model,X_test=X_test,Y_test_original=Y_test_original)


#初始化参数
top_words=140000
max_words=40
batch_size=32
epochs=8
hidden_dim=[128]
show_confusion_matrix=True
show_loss=True

train_fuc(mode='RNN',batch_size=batch_size,epochs=8)
train_fuc(mode='MLP',epochs=epochs)
train_fuc(mode='CNN',epochs=epochs)
train_fuc(mode='LSTM',epochs=epochs)
train_fuc(mode='GRU',epochs=epochs)
train_fuc(mode='CNN+LSTM',epochs=epochs)
train_fuc(mode='BiLSTM',epochs=epochs)
train_fuc(mode='TextCNN',epochs=8)
train_fuc(mode='Transformer',epochs=8)
train_fuc(mode='PositionalEmbedding+Transformer',batch_size=batch_size,epochs=3)

df_eval.assign(s=df_eval.sum(axis=1))#['s'].idxmax()
bar_width = 0.4
colors=['c', 'b', 'g', 'tomato', 'm', 'y', 'lime', 'k','orange','pink','grey','tan','gold','r']
fig, ax = plt.subplots(2,2,figsize=(10,8),dpi=128)
for i,col in enumerate(df_eval.columns):
    n=int(str('22')+str(i+1))
    plt.subplot(n)
    df_col=df_eval[col]
    m =np.arange(len(df_col))
    plt.bar(x=m,height=df_col.to_numpy(),width=bar_width,color=colors)
    
    #plt.xlabel('Methods',fontsize=12)
    names=df_col.index
    plt.xticks(range(len(df_col)),names,fontsize=10)
    plt.xticks(rotation=40)
    plt.ylabel(col,fontsize=14)
    
plt.tight_layout()
#plt.savefig('柱状图.jpg',dpi=512)
plt.show()




# model=build_model(max_words=max_words,mode='LSTM',hidden_dim=hidden_dim)
# history=model.fit(X,Y,batch_size=batch_size,epochs=3,verbose=0)

# # 将文本转换为序列
# Z_test = tok.texts_to_sequences(test_df['content'].values)

# # 填充序列到固定长度
# Z_test = keras.preprocessing.sequence.pad_sequences(Z_test, maxlen=40)

# # 使用模型进行预测
# prob = model.predict(Z_test)
# pred = np.argmax(prob, axis=1)

# # 将预测结果保存到CSV文件
# predictions_df = pd.DataFrame({'id': test_df['id'], 'predictedcategory': pred})
# predictions_df.to_csv('results.csv', index=False)