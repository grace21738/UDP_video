# 計算機網路Hw03

> ntnu40771107H 資工四 簡郁宸

### 環境

+ Ubuntu 20.04 x64
+ OpenCV 3.4.15

### 執行方式

1. 輸入make 

   ```
   $ make
   ```

2.  **優先執行agent**，執行命令如下 :

   ```
   $ ./agent <sender IP> <recv IP> <sender port> <agent port> <recv port> <loss_rate>
   ```

3. **再執行receiver** ，執行命令如下 :

   ```
   $ ./receiver <recv port> <agent port>
   ```

4.  ==**最後執行sender**，執行命令如下 :==

   > 由於影片播放的速度非常慢(在loss rate = 0的情況下，電腦上實測大概半小時 100 frame )，若想要加快速度播放，則可以利用<frame rate>減少影片播放時間。若frame rate = 3 ，則frame會是每三幀取一幀，也就是原本1200 frame現在只取400 frame。
   >
   > **<frame rate> range 為1 ~ 200 ( default為1 )**
   
   ```
   $ ./sender <video name> <sender port> <agent port> <frame rate>
   ```

### 注意事項

+ **播放的影片需和sender.exe放在同個目錄**
+ 若**傳送sequence number為1 的packet 被drop掉**，且大於1的sequence number packet receiver有收到，則receiver會**回傳ack 0**
+ timer設定為1秒，若timeout則 threshold 為window_size的一半，但不小於1

### receiver buffer設置

+ 一開始的receiver一開始的buffer為4，主要傳送video 基本資訊

  | 總共多少frame | video width | video height | 一個frame多少bytes |
  | ------------- | ----------- | ------------ | ------------------ |

+ 後面設置的buffer則為 ((一個frame多少bytes)/1000)