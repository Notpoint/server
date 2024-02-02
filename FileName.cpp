#pragma comment(lib,"ws2_32.lib")
// 定义端口号和缓冲区大小
// 导入必要的头文件
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <stdio.h>
#include <mysql.h>
#include <cstdio>
#define HOST "127.0.0.1"
#define USER "root"
#define PASS "123456"
#define DB "parks"
#define PORTT 3306
// 定义表名
#define TABLE "login"
#define TABLEE "contents"


// 定义服务器端口和最大客户端数量
#define PORT 8888
#define MAX_CLIENTS 10

// 定义一个客户端结构体，包含套接字和地址信息
struct Client {
    SOCKET sock;
    sockaddr_in addr;
    bool accept=false;
    char name[100];
};

// 定义一个全局的客户端数组，用于存储已连接的客户端
std::vector<Client> clients;

// 定义一个互斥锁，用于同步对客户端数组的访问
std::mutex mtx;

void closeconnect(Client &client) {
    closesocket(client.sock);

    // 从客户端数组中移除客户端
    mtx.lock();
    for (auto it = clients.begin(); it != clients.end(); it++) {
        if (it->sock == client.sock) {
            clients.erase(it);
            break;
        }
    }
    mtx.unlock();
}

// 定义一个函数，用于处理每个客户端的通信
void handle_client(Client &client) {
    

    // 获取客户端的IP地址和端口号
    char buffer[1024];
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client.addr.sin_addr, ip, sizeof(ip));
    int port = ntohs(client.addr.sin_port);

    // 打印客户端的连接信息
    std::cout << "Client " << ip << ":"  <<port <<" connected.\n";

    memset(buffer, 0, sizeof(buffer));

    // 接收客户端发送的数据
    int bytes_received = recv(client.sock, buffer, sizeof(buffer), 0);

    // 如果接收到的字节数为0，表示客户端断开连接
    if (bytes_received == 0) {
        std::cout << "Client " << ip << ":" << port << " disconnected.\n";
        closeconnect(client);
        return;
    }

    // 如果接收到的字节数为-1，表示出错
    if (bytes_received == -1) {
        std::cerr << "Error in recv().\n";
        
        // 从客户端数组中移除客户端
        closeconnect(client);
        return;
    }



    //数据库
    MYSQL* conn = mysql_init(NULL);
    if (mysql_real_connect(conn, HOST, USER, PASS, DB, PORTT, NULL, 0)) {
        char sql[100];
        sprintf_s(sql, "select * from %s", TABLE);
        if (mysql_query(conn, sql) == 0)
        {
            
            // 获取结果集
            MYSQL_RES* res = mysql_store_result(conn);
            if (res)
            {
                // 获取字段数
                int num_fields = mysql_num_fields(res);
                
                char buffer1[1024];
                char buffer2[1024];
                int space = -1;
                for (int i = 0; i < 1024; i++) {
                    if (buffer[i] == ' ') {
                        space = i;
                        break;
                    }
                }

                // split the buffer array by space
                if (space != -1) {
                    for (int i = 0; i < space; i++) {
                        buffer1[i] = buffer[i];
                    }
                    buffer1[space] = '\0';
                    for (int i = 0; i < strlen(buffer) - space - 1; i++) {
                        buffer2[i] = buffer[space + i + 1];
                    }
                    buffer2[strlen(buffer) - space - 1] = '\0';
                }
                else {
                    closeconnect(client);
                    mysql_close(conn);
                    return;
                }
                
                
                // 获取每一行的数据
                MYSQL_ROW row;
                
                while ((row = mysql_fetch_row(res)))
                {
                        if (!strcmp(buffer1, row[0]) && !strcmp(buffer2, row[1])) {
                            strcpy_s(client.name, sizeof(client.name), buffer1);
                            char accept[10]="accept";
                            std::cout << "验证通过";
                            client.accept = true;
                            send(client.sock, accept, strlen(accept), 0);
                            break;
                        }
                    
                    
                }
                // 释放结果集
                mysql_free_result(res);
                if (!client.accept) {
                    std::cout << "验证失败";
                    closeconnect(client);
                    mysql_close(conn);
                    return;
                }
            }
        }
        else
        {
            printf("查询失败！\n");
        }
        // 关闭数据库连接
        //mysql_close(conn);
    }
    else {
        std::cout << "数据库连接失败";
        closeconnect(client);
        mysql_close(conn);
        return;

    }
    
    MYSQL_ROW row2;
    char sql2[100];
    sprintf_s(sql2, "select * from %s", TABLEE);
    if (mysql_query(conn, sql2) == 0)
    {

        // 获取结果集
        MYSQL_RES* res2 = mysql_store_result(conn);
        if (res2)
        {
            // 获取字段数
            int num_fields = mysql_num_fields(res2);
            char buffer3[1024];
            while ((row2 = mysql_fetch_row(res2)))
            {
                    strcpy_s(buffer3, sizeof(buffer3), row2[0]);
                    Sleep(50);
                    send(client.sock, buffer3, strlen(buffer3), 0);
                    
                


            }
            // 释放结果集
            mysql_free_result(res2);
        }
    }
    //mysql_close(conn);




    

    // 循环接收和发送数据，直到客户端断开连接或出错
    while (true) {
        // 清空缓冲区
        memset(buffer, 0, sizeof(buffer));

        // 接收客户端发送的数据
        int bytes_received = recv(client.sock, buffer, sizeof(buffer), 0);

        // 如果接收到的字节数为0，表示客户端断开连接
        if (bytes_received == 0) {
            std::cout << "Client " << ip << ":" << port << " disconnected.\n";
            break;
        }

        // 如果接收到的字节数为-1，表示出错
        if (bytes_received == -1) {
            std::cerr << "Error in recv().\n";
            break;
        }

        // 打印接收到的数据
        std::cout << "Received from " << ip << ":" << port << ": " << buffer << "\n";
         // arr是要存放port的字符数组
        char buffernext[1024];

        for (int i = 0; i < strlen(buffer); i++) {
            buffernext[i + strlen(client.name)+2] = buffer[i];
        }
        buffernext[strlen(client.name)] = ':';
        buffernext[strlen(client.name)+1] = ' ';
        buffernext[strlen(buffer) + strlen(client.name)+2] = '\0';
        for (int i = 0; i < strlen(client.name); i++) {
            buffernext[i] = client.name[i];
        }
        
        mtx.lock();
        
        for (int i = 0; i < clients.size(); i++) {
            if (clients[i].accept = true) {
                send(clients[i].sock, buffernext, bytes_received + strlen(client.name)+2, 0);
                char* next = NULL;
                //把buffer中的数据分割为一个字段，用逗号分隔
                char* name = strtok_s(buffernext, ",", &next);
                //创建一个预处理语句的句柄
                MYSQL_STMT* stmt = mysql_stmt_init(conn);
                if (stmt)
                {
                    //把sql语句绑定到预处理语句的句柄上
                    std::string sql = "INSERT INTO contents (content) VALUES (?)";
                    if (mysql_stmt_prepare(stmt, sql.c_str(), sql.length()) == 0)
                    {
                        //创建一个MYSQL_BIND类型的数组，用来绑定参数
                        MYSQL_BIND bind[1];
                        //清零数组
                        memset(bind, 0, sizeof(bind));
                        //绑定第一个参数，name
                        bind[0].buffer_type = MYSQL_TYPE_STRING;
                        bind[0].buffer = name;
                        bind[0].buffer_length = strlen(name);
                        //把参数绑定到预处理语句上
                        if (mysql_stmt_bind_param(stmt, bind) == 0)
                        {
                            //执行预处理语句，插入数据到数据库中
                            if (mysql_stmt_execute(stmt) == 0)
                            {
                                std::cout << "Inserted data successfully." << std::endl;
                            }
                            else
                            {
                                std::cout << "Failed to execute statement: " << mysql_stmt_error(stmt) << std::endl;
                            }
                        }
                        else
                        {
                            std::cout << "Failed to bind parameters: " << mysql_stmt_error(stmt) << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "Failed to prepare statement: " << mysql_stmt_error(stmt) << std::endl;
                    }
                    //关闭预处理语句的句柄
                    mysql_stmt_close(stmt);
                }
                else
                {
                    std::cout << "Failed to initialize statement: " << mysql_error(conn) << std::endl;
                }
               
            }
            

        }
        mtx.unlock();
        // 将接收到的数据原样发送回客户端
        
    }

    // 关闭客户端的套接字
    closeconnect(client);
}

// 主函数
int main() {

    
    
    // 初始化Winsock库
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::cerr << "Error in WSAStartup().\n";
        return -1;
    }

    // 创建服务器端的套接字
    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCKET) {
        std::cerr << "Error in socket().\n";
        WSACleanup();
        return -1;
    }
    
    // 设置服务器端的地址信息
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 绑定服务器端的套接字和地址信息
    if (bind(server_sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Error in bind().\n";
        closesocket(server_sock);
        WSACleanup();
        return -1;
    }

    // 监听服务器端的套接字
    if (listen(server_sock, MAX_CLIENTS) == SOCKET_ERROR) {
        std::cerr << "Error in listen().\n";
        closesocket(server_sock);
        WSACleanup();
        return -1;
    }

    // 打印服务器端的监听信息
    std::cout << "Server listening on port " << PORT << ".\n";

    // 循环接受客户端的连接请求
    while (true) {
        // 定义一个客户端结构体，用于存储客户端的套接字和地址信息
        Client client;
        int client_addr_size = sizeof(client.addr);

        // 接受客户端的连接请求
        client.sock = accept(server_sock, (sockaddr*)&client.addr, &client_addr_size);
        if (client.sock == INVALID_SOCKET) {
            std::cerr << "Error in accept().\n";
            break;
        }

        // 将客户端添加到客户端数组中
        mtx.lock();
        clients.push_back(client);
        mtx.unlock();

        // 创建一个线程，用于处理客户端的通信
        std::thread t(handle_client, std::ref(client));
        t.detach();
    }

    // 关闭服务器端的套接字
    closesocket(server_sock);

    // 清理Winsock库
    WSACleanup();
    
    // 返回0
    return 0;
    
    
}

