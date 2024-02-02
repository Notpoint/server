#pragma comment(lib,"ws2_32.lib")
// ����˿ںźͻ�������С
// �����Ҫ��ͷ�ļ�
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
// �������
#define TABLE "login"
#define TABLEE "contents"


// ����������˿ں����ͻ�������
#define PORT 8888
#define MAX_CLIENTS 10

// ����һ���ͻ��˽ṹ�壬�����׽��ֺ͵�ַ��Ϣ
struct Client {
    SOCKET sock;
    sockaddr_in addr;
    bool accept=false;
    char name[100];
};

// ����һ��ȫ�ֵĿͻ������飬���ڴ洢�����ӵĿͻ���
std::vector<Client> clients;

// ����һ��������������ͬ���Կͻ�������ķ���
std::mutex mtx;

void closeconnect(Client &client) {
    closesocket(client.sock);

    // �ӿͻ����������Ƴ��ͻ���
    mtx.lock();
    for (auto it = clients.begin(); it != clients.end(); it++) {
        if (it->sock == client.sock) {
            clients.erase(it);
            break;
        }
    }
    mtx.unlock();
}

// ����һ�����������ڴ���ÿ���ͻ��˵�ͨ��
void handle_client(Client &client) {
    

    // ��ȡ�ͻ��˵�IP��ַ�Ͷ˿ں�
    char buffer[1024];
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client.addr.sin_addr, ip, sizeof(ip));
    int port = ntohs(client.addr.sin_port);

    // ��ӡ�ͻ��˵�������Ϣ
    std::cout << "Client " << ip << ":"  <<port <<" connected.\n";

    memset(buffer, 0, sizeof(buffer));

    // ���տͻ��˷��͵�����
    int bytes_received = recv(client.sock, buffer, sizeof(buffer), 0);

    // ������յ����ֽ���Ϊ0����ʾ�ͻ��˶Ͽ�����
    if (bytes_received == 0) {
        std::cout << "Client " << ip << ":" << port << " disconnected.\n";
        closeconnect(client);
        return;
    }

    // ������յ����ֽ���Ϊ-1����ʾ����
    if (bytes_received == -1) {
        std::cerr << "Error in recv().\n";
        
        // �ӿͻ����������Ƴ��ͻ���
        closeconnect(client);
        return;
    }



    //���ݿ�
    MYSQL* conn = mysql_init(NULL);
    if (mysql_real_connect(conn, HOST, USER, PASS, DB, PORTT, NULL, 0)) {
        char sql[100];
        sprintf_s(sql, "select * from %s", TABLE);
        if (mysql_query(conn, sql) == 0)
        {
            
            // ��ȡ�����
            MYSQL_RES* res = mysql_store_result(conn);
            if (res)
            {
                // ��ȡ�ֶ���
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
                
                
                // ��ȡÿһ�е�����
                MYSQL_ROW row;
                
                while ((row = mysql_fetch_row(res)))
                {
                        if (!strcmp(buffer1, row[0]) && !strcmp(buffer2, row[1])) {
                            strcpy_s(client.name, sizeof(client.name), buffer1);
                            char accept[10]="accept";
                            std::cout << "��֤ͨ��";
                            client.accept = true;
                            send(client.sock, accept, strlen(accept), 0);
                            break;
                        }
                    
                    
                }
                // �ͷŽ����
                mysql_free_result(res);
                if (!client.accept) {
                    std::cout << "��֤ʧ��";
                    closeconnect(client);
                    mysql_close(conn);
                    return;
                }
            }
        }
        else
        {
            printf("��ѯʧ�ܣ�\n");
        }
        // �ر����ݿ�����
        //mysql_close(conn);
    }
    else {
        std::cout << "���ݿ�����ʧ��";
        closeconnect(client);
        mysql_close(conn);
        return;

    }
    
    MYSQL_ROW row2;
    char sql2[100];
    sprintf_s(sql2, "select * from %s", TABLEE);
    if (mysql_query(conn, sql2) == 0)
    {

        // ��ȡ�����
        MYSQL_RES* res2 = mysql_store_result(conn);
        if (res2)
        {
            // ��ȡ�ֶ���
            int num_fields = mysql_num_fields(res2);
            char buffer3[1024];
            while ((row2 = mysql_fetch_row(res2)))
            {
                    strcpy_s(buffer3, sizeof(buffer3), row2[0]);
                    Sleep(50);
                    send(client.sock, buffer3, strlen(buffer3), 0);
                    
                


            }
            // �ͷŽ����
            mysql_free_result(res2);
        }
    }
    //mysql_close(conn);




    

    // ѭ�����պͷ������ݣ�ֱ���ͻ��˶Ͽ����ӻ����
    while (true) {
        // ��ջ�����
        memset(buffer, 0, sizeof(buffer));

        // ���տͻ��˷��͵�����
        int bytes_received = recv(client.sock, buffer, sizeof(buffer), 0);

        // ������յ����ֽ���Ϊ0����ʾ�ͻ��˶Ͽ�����
        if (bytes_received == 0) {
            std::cout << "Client " << ip << ":" << port << " disconnected.\n";
            break;
        }

        // ������յ����ֽ���Ϊ-1����ʾ����
        if (bytes_received == -1) {
            std::cerr << "Error in recv().\n";
            break;
        }

        // ��ӡ���յ�������
        std::cout << "Received from " << ip << ":" << port << ": " << buffer << "\n";
         // arr��Ҫ���port���ַ�����
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
                //��buffer�е����ݷָ�Ϊһ���ֶΣ��ö��ŷָ�
                char* name = strtok_s(buffernext, ",", &next);
                //����һ��Ԥ�������ľ��
                MYSQL_STMT* stmt = mysql_stmt_init(conn);
                if (stmt)
                {
                    //��sql���󶨵�Ԥ�������ľ����
                    std::string sql = "INSERT INTO contents (content) VALUES (?)";
                    if (mysql_stmt_prepare(stmt, sql.c_str(), sql.length()) == 0)
                    {
                        //����һ��MYSQL_BIND���͵����飬�����󶨲���
                        MYSQL_BIND bind[1];
                        //��������
                        memset(bind, 0, sizeof(bind));
                        //�󶨵�һ��������name
                        bind[0].buffer_type = MYSQL_TYPE_STRING;
                        bind[0].buffer = name;
                        bind[0].buffer_length = strlen(name);
                        //�Ѳ����󶨵�Ԥ���������
                        if (mysql_stmt_bind_param(stmt, bind) == 0)
                        {
                            //ִ��Ԥ������䣬�������ݵ����ݿ���
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
                    //�ر�Ԥ�������ľ��
                    mysql_stmt_close(stmt);
                }
                else
                {
                    std::cout << "Failed to initialize statement: " << mysql_error(conn) << std::endl;
                }
               
            }
            

        }
        mtx.unlock();
        // �����յ�������ԭ�����ͻؿͻ���
        
    }

    // �رտͻ��˵��׽���
    closeconnect(client);
}

// ������
int main() {

    
    
    // ��ʼ��Winsock��
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::cerr << "Error in WSAStartup().\n";
        return -1;
    }

    // �����������˵��׽���
    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCKET) {
        std::cerr << "Error in socket().\n";
        WSACleanup();
        return -1;
    }
    
    // ���÷������˵ĵ�ַ��Ϣ
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // �󶨷������˵��׽��ֺ͵�ַ��Ϣ
    if (bind(server_sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Error in bind().\n";
        closesocket(server_sock);
        WSACleanup();
        return -1;
    }

    // �����������˵��׽���
    if (listen(server_sock, MAX_CLIENTS) == SOCKET_ERROR) {
        std::cerr << "Error in listen().\n";
        closesocket(server_sock);
        WSACleanup();
        return -1;
    }

    // ��ӡ�������˵ļ�����Ϣ
    std::cout << "Server listening on port " << PORT << ".\n";

    // ѭ�����ܿͻ��˵���������
    while (true) {
        // ����һ���ͻ��˽ṹ�壬���ڴ洢�ͻ��˵��׽��ֺ͵�ַ��Ϣ
        Client client;
        int client_addr_size = sizeof(client.addr);

        // ���ܿͻ��˵���������
        client.sock = accept(server_sock, (sockaddr*)&client.addr, &client_addr_size);
        if (client.sock == INVALID_SOCKET) {
            std::cerr << "Error in accept().\n";
            break;
        }

        // ���ͻ�����ӵ��ͻ���������
        mtx.lock();
        clients.push_back(client);
        mtx.unlock();

        // ����һ���̣߳����ڴ���ͻ��˵�ͨ��
        std::thread t(handle_client, std::ref(client));
        t.detach();
    }

    // �رշ������˵��׽���
    closesocket(server_sock);

    // ����Winsock��
    WSACleanup();
    
    // ����0
    return 0;
    
    
}

