把连接抽象成Connection类，不再是裸fd。每一个连接对应一个connection

解决的问题：
fd没有封装；
后续加功能拓展会很麻烦