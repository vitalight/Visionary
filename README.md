# Visionary

This is the SJTU Computer Vision final homework, a Photoshop-like software named Visionary.

## Todo List
- 总体功能
  - [ ] timming取消，或者放mainwindow
  - [ ] thinning not working
  - [x] 右边输入栏（像素值）
  - [x] 实时显示
  - [x] 双图片显示
  - [x] 历史操作显示


- 彩色图像处理
  - [x] 三通道分离 
  - [x] 彩色  -->灰度 
  - [x] 色相/饱和度/亮度调节 
  - [ ] 色阶调整（选做）
- 二值化
  - [x] Otus 
  - [x] 手动调节：双阈值，实时 
- 代数与几何操作
  - [x] 加、减、乘、剪裁 
  - [x] 缩放、旋转 
- 对比度调节
  - [ ] 线性及分段线性调整
  - [ ] 非线性调整：对数、指数（系数可调）
  - [x] 图像的直方图显示 
  - [x] 直方图均衡化 
- 平滑滤波器（卷积核允许用户自定义）
  - [x] 均值 
  - [x] 中值 
  - [x] 高斯
  - [x] 自定义 
- 边缘检测
  - [x] Sobel、拉普拉斯、canny 
- 霍夫变换（选做）
  - [ ] 检测直线和圆
- 二值数学形态学（结构元允许用户自定义）
  - [x] 膨胀、腐蚀 V
  - [x] 开、闭 V
  - [x] 细化、粗化 V?
  - [x] 距离变换
  - [x] 骨架 X有bug
  - [x] 骨架重构 X
  - [x] 二值形态学重构
- 灰度数学形态学
  - [x] 膨胀、腐蚀
  - [x] 开、闭
  - [x] 形态学重构
  - [ ] 分水岭算法



##　Underlying problem

showResponseTime() don't work well

图像缩放旋转的时候算入透明度



##  History

17周周五(1月5日)答辩