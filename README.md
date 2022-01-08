# 計算機網路Hw03

> ntnu40771107H 資工四 簡郁宸

## 執行

### + 環境

+ Ubuntu 20.04 x64
+ OpenCV 3.4.15

### + 執行方式

1. 輸入make 

   ```
   $ make
   ```

2. **優先執行agent**，執行命令如下 :

   ```
   $ ./agent <sender IP> <recv IP> <sender port> <agent port> <recv port> <loss_rate>
   ```

   >  Example: 	./agent 127.0.0.1 127.0.0.1 7777 1234 8888 0.001

3. **再執行receiver** ，執行命令如下 :

   ```
   $ ./receiver <recv port> <agent port>
   ```

   > Example: 	./receiver 8888 1234

4.  **最後執行sender**，執行命令如下 :

   > 由於影片播放的速度非常慢(在loss rate = 0的情況下，電腦上實測大概半小時 100 frame )，若想要加快速度播放，則可以利用<frame rate>減少影片播放時間。若frame rate = 3 ，則frame會是每三幀取一幀，也就是原本1200 frame現在只取400 frame。
   >
   > **<frame rate> range 為1 ~ 200 ( default為1 )**
   
   ```
   $ ./sender <video name> <sender port> <agent port> <frame rate>
   ```
   
   >  Example: 	./sender video.mpg 7777 1234 10

### + 注意事項

+ **播放的影片需和sender.exe放在同個目錄**
+ timer設定為1秒，若timeout則 threshold 為window_size的一半，但不小於1，且window_size=1。



## Flow chart

### Sender

<img src="C:\Users\grace\Desktop\課程\四上課程\計網\hw03\sender.png" alt="sender" style="zoom:67%;" />

#### 說明

+ window_size由一開始，使用vector儲存frame的小packet檔案，每收到期待的seqNum的ack時，vector裡收到的packet會被清掉，並且push進packet。
+ 當成功收到期待的seqNum時，window_size 增加，並且塞入packet至vector直到vector裡的packet數量為window_size為止。
+ 為了不讓sender被rcvfrom() block 住，使用select() 確認有東西傳送過來，再呼叫rcvfrom()。
+ 傳送一次最大的packet 為1000 byte。
+ Timer使用pthread實現，若時間到時會回傳一個time_out signal。



### Agent

<img src="C:\Users\grace\Desktop\課程\四上課程\計網\hw03\agent.png" alt="agent" style="zoom: 80%;" />

#### 說明

+ 設置data loss並且以random的方式控制是否丟掉sender的packet。
+ 從receiver收到的Ack一律pass 給sender。
+ 若收到finish packet (finish ack)，則會直接pass給receiver ( sender )。

### Receiver

<img src="C:\Users\grace\Desktop\課程\四上課程\計網\hw03\receiver.png" alt="receiver" style="zoom:67%;" />

#### 說明

+ 第一個buffer size為1，主要接收video的基本資訊( 總frame數、影片長寬、一個frame需要多少packet傳送 )。
+ 第二個buffer size則為一個frame的大小。
+ 每當buffer滿時播出一個frame。