import pymysql
import random
from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes
from Crypto.Util.Padding import pad, unpad
import base64

local_table = {}
key = get_random_bytes(16)
base_iv = get_random_bytes(16)

def AES_ENC(plaintext, iv):
    # AES加密
    aes = AES.new(key, AES.MODE_CBC, iv=iv)
    padded_data = pad(plaintext, AES.block_size, style='pkcs7')
    ciphertext = aes.encrypt(padded_data)
    return ciphertext

def AES_DEC(ciphertext, iv):
    # AES解密
    aes = AES.new(key, AES.MODE_CBC, iv=iv)
    padded_data = aes.decrypt(ciphertext)
    plaintext = unpad(padded_data, AES.block_size, style='pkcs7')
    return plaintext

def Random_Encrypt(plaintext):
    # 随机生成iv来保证加密结果的随机性
    iv = get_random_bytes(16)
    ciphertext = AES_ENC(iv + AES_ENC(plaintext.encode('utf-8'), iv), base_iv)
    ciphertext = base64.b64encode(ciphertext)
    return ciphertext.decode('utf-8')

def Random_Decrypt(ciphertext):
    plaintext = AES_DEC(base64.b64decode(ciphertext.encode('utf-8')) ,base_iv)
    plaintext = AES_DEC(plaintext[16:],plaintext[:16])
    return plaintext.decode('utf-8')

def CalPos(plaintext, strategy='random'):
    # 插入plaintext，返回对应的Pos
    presum = sum([v for k, v in local_table.items() if k < plaintext])
    old_count = local_table.get(plaintext, 0)
    new_count = old_count + 1
    local_table[plaintext] = new_count

    if old_count == 0:
        return presum

    if strategy == 'left_edge':
        # 压力测试模式：总是插入到相同明文区间左端，快速耗尽局部编码间隔。
        return presum
    if strategy == 'right_edge':
        return presum + new_count - 1
    if strategy == 'middle':
        return presum + new_count // 2

    return random.randint(presum, presum + new_count - 1)

def GetLeftPos(plaintext):
    return sum([v for k, v in local_table.items() if k < plaintext])

def GetRightPos(plaintext):
    return sum([v for k, v in local_table.items() if k <= plaintext])

def OpenConn():
    return pymysql.connect(host='localhost', user='user', passwd='123456', database='test_db')

def Snapshot(cur):
    cur.execute("select encoding, ciphertext from example order by encoding, ciphertext")
    rows = cur.fetchall()
    result = {}
    for encoding, ciphertext in rows:
        result[ciphertext] = encoding
    return result

def PrintTable(cur, title, max_rows=20):
    cur.execute("select encoding, ciphertext from example order by encoding, ciphertext")
    rows = cur.fetchall()
    print("")
    print("[{}] total rows = {}".format(title, len(rows)))
    for index, row in enumerate(rows[:max_rows], start=1):
        encoding = row[0]
        ciphertext = row[1]
        try:
            plaintext = Random_Decrypt(ciphertext)
        except Exception:
            plaintext = "<cannot decrypt: old key or dirty table>"
        print("{:03d}: encoding={}, plaintext={}, ciphertext={}...".format(
            index, encoding, plaintext, ciphertext[:24]
        ))
    if len(rows) > max_rows:
        print("... only first {} rows are shown".format(max_rows))

def PrintGapStats(cur, title):
    cur.execute("select encoding from example order by encoding")
    encodings = [row[0] for row in cur.fetchall()]
    if len(encodings) < 2:
        print("[{}] not enough rows to calculate gaps".format(title))
        return

    min_gap = None
    min_index = 0
    for index in range(1, len(encodings)):
        gap = encodings[index] - encodings[index - 1]
        if min_gap is None or gap < min_gap:
            min_gap = gap
            min_index = index

    print("[{}] min encoding gap = {}, between row {} and row {}".format(
        title, min_gap, min_index, min_index + 1
    ))

def InsertWithTrace(plaintext, step=None, print_table=False, strategy='random'):
    ciphertext = Random_Encrypt(plaintext)
    pos = CalPos(plaintext, strategy=strategy)
    conn = OpenConn()
    cur = conn.cursor()

    before = Snapshot(cur)
    if step is None:
        label = "insert"
    else:
        label = "step {}".format(step)
    print("")
    print("===== {}: plaintext={}, pos={}, local_table={} =====".format(
        label, plaintext, pos, local_table
    ))
    print("insert strategy: {}".format(strategy))

    # 展开 pro_insert 的核心逻辑，便于观察 FHInsert 和 FHUpdate。
    cur.execute("select FHInsert(%s, %s)", (pos, ciphertext))
    inserted_encoding = cur.fetchone()[0]
    print("FHInsert returned: {}".format(inserted_encoding))

    cur.execute("insert into example values (%s, %s)", (inserted_encoding, ciphertext))

    if inserted_encoding == 0:
        # 返回 0 表示触发 Recode，需要更新数据库中受影响区间的 encoding。
        cur.execute("select FHStart(), FHEnd()")
        start_update, end_update = cur.fetchone()
        print("[RECODE] update interval: [{}, {})".format(start_update, end_update))

        cur.execute(
            "select count(*) from example "
            "where (encoding >= FHStart() and encoding < FHEnd()) or encoding = 0"
        )
        affected = cur.fetchone()[0]
        print("[RECODE] rows to refresh with FHUpdate: {}".format(affected))

        cur.execute(
            "update example "
            "set encoding = FHUpdate(ciphertext) "
            "where (encoding >= FHStart() and encoding < FHEnd()) or encoding = 0"
        )
        print("[RECODE] FHUpdate executed")
    else:
        print("[NORMAL] no FHUpdate is needed for this insert")

    conn.commit()

    after = Snapshot(cur)
    changed = []
    for old_ciphertext, old_encoding in before.items():
        new_encoding = after.get(old_ciphertext)
        if new_encoding is not None and new_encoding != old_encoding:
            changed.append((old_encoding, new_encoding, old_ciphertext))

    if changed:
        print("[ENCODING UPDATED] existing rows changed: {}".format(len(changed)))
        for old_encoding, new_encoding, old_ciphertext in changed[:10]:
            print("  {} -> {}, ciphertext={}...".format(
                old_encoding, new_encoding, old_ciphertext[:24]
            ))
    else:
        print("[ENCODING UPDATED] no existing row changed")

    if print_table:
        PrintTable(cur, "table after {}".format(label))
        PrintGapStats(cur, "gap stats after {}".format(label))

    conn.close()

def Search(left, right):
    # 搜索[left,right]中的信息
    left_pos = GetLeftPos(left)
    right_pos = GetRightPos(right)
    # 连接数据库
    conn = OpenConn()
    cur = conn.cursor()

    total_count = sum(local_table.values())
    if right_pos >= total_count:
        # FHSearch(pos) 只能查已有下标。right_pos 等于总数量时表示右开边界在最后一条之后。
        cur.execute(
            "select ciphertext from example where encoding >= FHSearch(%s) order by encoding",
            (left_pos,)
        )
    else:
        cur.execute(
            "select ciphertext from example where encoding >= FHSearch(%s) and encoding < FHSearch(%s) order by encoding",
            (left_pos, right_pos)
        )

    rest = cur.fetchall()
    print("Search({}, {}) returned {} rows".format(left, right, len(rest)))
    for x in rest:
        try:
            plaintext = Random_Decrypt(x[0])
        except Exception:
            plaintext = "<cannot decrypt: old key or dirty table>"
        print("ciphtertext: {} plaintext: {}".format(x[0], plaintext))
    conn.close()

def RepeatedInsertTest(value='apple', times=140, strategy='left_edge'):
    random.seed(2026)
    print("===== Repeated insert test =====")
    print("value={}, times={}, strategy={}".format(value, times, strategy))
    print("strategy left_edge is used to force recode/update in a short run")
    for i in range(1, times + 1):
        # M = 128，接近第 128 次时最容易观察到叶子结点分裂相关现象。
        verbose = i <= 5 or i in [60, 61, 62, 63, 64, 126, 127, 128, 129, times]
        if i == 128:
            print("")
            print("[SPLIT EXPECTED] step 128 reaches M=128; LeafNode::rebalance should be triggered in the UDF")
        InsertWithTrace(value, step=i, print_table=verbose, strategy=strategy)

    print("")
    print("===== Search result for the repeated plaintext =====")
    Search(value, value)

if __name__ == '__main__':
    # 运行前建议先在 MySQL 中清空 example 表，并重启 MySQL 以清空 UDF 的全局编码树状态。
    # use test_db;
    # delete from example;
    RepeatedInsertTest('apple', 140, strategy='left_edge')
