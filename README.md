# Visionary

This is the SJTU Computer Vision final homework, a Photoshop-like software named Visionary.



眼球：绿色通道、色阶



## Test Record

- [x] delete 新建
- [x] 缩放 改slider 初始值似乎不对
- [x] 坏死了， 估计是>8之后再撤销就不对
- [x] 界面优化完成
- [x] 双阈值初始值
- [ ] 灰度图像HSB失败
- [ ] 分水岭算法/霍夫变换
- [ ] 眼球图片处理
- [x] out of memory
- [x] 形态学重构



## Bug Repair

- [x] timming取消，或者放mainwindow
- [x] 每次都要重置为0 kernel
- [x] 指数变换、log的曲线不太对
- [x] 细化算法 正确？
- [x] 骨架算法
- [x] 骨架重构
- [ ] watershed error output



## Todo List

- 总体功能
  - [x] timming取消，或者放mainwindow
  - [x] 每次都要重置为0 kernel
  - [x] 指数变换、log的曲线不太对
  - [x] 右边输入栏（像素值）
  - [x] 实时显示
  - [x] 双图片显示
  - [x] 历史操作显示


- 彩色图像处理
  - [x] 三通道分离 
  - [x] 彩色  -->灰度 
  - [x] 色相/饱和度/亮度调节 
  - [x] 色阶调整（选做）
- 二值化
  - [x] Otus 
  - [x] 手动调节：双阈值，实时 
- 代数与几何操作
  - [x] 加、减、乘、剪裁 
  - [x] 缩放、旋转 
- 对比度调节
  - [x] 线性及分段线性调整
  - [x] 非线性调整：对数、指数（系数可调）
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
  - [x] 膨胀、腐蚀 
  - [x] 开、闭 
  - [x] 细化、粗化 
  - [x] 距离变换
  - [x] 骨架
  - [x] 骨架重构 
  - [x] 二值形态学重构
- 灰度数学形态学
  - [x] 膨胀、腐蚀
  - [x] 开、闭
  - [x] 形态学重构
  - [ ] 分水岭算法




##  History

17周周五(1月5日)答辩