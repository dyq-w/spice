# -*- coding: utf-8 -*-
"""
Created on Tue Sep 19 21:12:26 2023

@author: 段永强
"""

import matplotlib.pyplot as plt
import numpy as np



def loadData(file):
 inFile = open(file, 'r')#以只读方式打开某filename文件
 #定义2个空的list，用来存放文件中的数据
 x = np.linspace(0.001,0.4,400)
 y1 = []
 y2 = []
 y3 = []
 y4 = [] 
 y5 = []
 for line in inFile:
     if line.split(':')[0]!='Time':
         flag=line.split()[1].split(':')[1]
         if line.split()[0] == 'c1':
             y1.append(float(flag))
         if line.split()[0] == 'c2':
             y2.append(float(flag))
         if line.split()[0] == 'c3':
             y3.append(float(flag))
         if line.split()[0] == 'c4':
             y4.append(float(flag))
         if line.split()[0] == 'c5':
             y5.append(float(flag))

 return (x, y1, y2, y3, y4, y5)


(x, y1, y2, y3, y4, y5) = loadData('E:\data.txt')
#print(x, y)


plt.figure()
plt.plot(x, y1)
plt.plot(x, y2)
plt.plot(x, y3)
plt.plot(x, y4)
plt.plot(x, y5)

plt.title('voltage changs over time')
plt.xlabel('time')
plt.ylabel('voltage')

# 显示图形
plt.show()



plt.figure(figsize=(10,10))
# 创建子图
plt.subplot(10, 1, 1)  # 2行1列的子图的第一个
plt.plot(x, y1)
plt.title('c1 plot')
plt.xlabel('time')
plt.ylabel('voltage')

plt.subplot(10, 1, 3)  # 2行1列的子图的第二个
plt.plot(x, y2)
plt.title('c2 plot')
plt.xlabel('time')
plt.ylabel('voltage')

plt.subplot(10, 1, 5)  # 2行1列的子图的第二个
plt.plot(x, y3)
plt.title('c3 plot')
plt.xlabel('time')
plt.ylabel('voltage')

plt.subplot(10, 1, 7)  # 2行1列的子图的第二个
plt.plot(x, y4)
plt.title('c4 plot')
plt.xlabel('time')
plt.ylabel('voltage')

plt.subplot(10, 1, 9)  # 2行1列的子图的第二个
plt.plot(x, y5)
plt.title('c5 plot')
plt.xlabel('time')
plt.ylabel('voltage')

# 显示图形
plt.show()