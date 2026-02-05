#include "http.hpp"

#define WWWROOT "./wwwroot/"
std::string RequestStr(const HttpRequest &req) 
{
    std::stringstream ss;
    ss << req._method << " " << req._path << " " << req._version << "\r\n";
    for (auto &it : req._params) {
        ss << it.first << ": " << it.second << "\r\n";
    }
    for (auto &it : req._headers) {
        ss << it.first << ": " << it.second << "\r\n";
    }
    ss << "\r\n";
    ss << req._body;
    return ss.str();
}
void Hello(const HttpRequest &req, HttpResponse *rsp)
{
    rsp->SetContent(RequestStr(req), "text/plain");
}
void Login(const HttpRequest &req, HttpResponse *rsp)
{
    rsp->SetContent(RequestStr(req), "text/plain");
}
void PutFile(const HttpRequest &req, HttpResponse *rsp)
{
    std::string pathname = WWWROOT + req._path;
    Util::WriteFile(pathname, req._body);
}
void DelFile(const HttpRequest &req, HttpResponse *rsp)
{
    rsp->SetContent(RequestStr(req), "text/plain");
}
int main()
{
    // 机器配置: 4c8g 用 8 线程; 2c2g 建议改为 2
    const int THREAD_CNT = 8;
    HttpServer server(8889, 300);  // 300 秒超时，便于压测
    server.SetThreadCnt(THREAD_CNT);
    server.SetBasedir(WWWROOT);
    server.Get("/hello", Hello);
    server.Post("/login", Login);
    server.Put("/1234.txt", PutFile);
    server.Delete("/1234.txt", DelFile);
    server.Listen();
 
    return 0;
}
// // main.cpp - 立即可以实现的版本
// #include "http.hpp"
// #include <unordered_map>
// #include <ctime>

// // 简单的内存Session管理（生产环境需要更安全的方式）
// std::unordered_map<std::string, std::string> user_sessions; // session_id -> username

// std::string GenerateSessionId() {
//     return std::to_string(time(nullptr)) + "_" + std::to_string(rand());
// }

// void RegisterPage(const HttpRequest& req, HttpResponse* rsp) {
//     std::string html = R"(<!DOCTYPE html>
// <html>
// <head>
//     <meta charset="utf-8">
//     <title>用户注册</title>
//     <style>
//         body { font-family: Arial; margin: 50px; }
//         .container { max-width: 400px; margin: 0 auto; }
//         input { width: 100%; padding: 10px; margin: 5px 0; }
//         button { width: 100%; padding: 10px; background: #4CAF50; color: white; border: none; }
//     </style>
// </head>
// <body>
//     <div class="container">
//         <h2>用户注册</h2>
//         <form action="/api/register" method="post">
//             <input type="text" name="username" placeholder="用户名" required>
//             <input type="password" name="password" placeholder="密码" required>
//             <input type="email" name="email" placeholder="邮箱">
//             <button type="submit">注册</button>
//         </form>
//         <p><a href="/login">已有账号？立即登录</a></p>
//     </div>
// </body>
// </html>)";
//     rsp->SetContent(html, "text/html");
// }

// void LoginPage(const HttpRequest& req, HttpResponse* rsp) {
//     std::string html = R"(<!DOCTYPE html>
// <html>
// <head>
//     <meta charset="utf-8">
//     <title>用户登录</title>
//     <style>/* 类似注册页面的样式 */</style>
// </head>
// <body>
//     <div class="container">
//         <h2>用户登录</h2>
//         <form action="/api/login" method="post">
//             <input type="text" name="username" placeholder="用户名" required>
//             <input type="password" name="password" placeholder="密码" required>
//             <button type="submit">登录</button>
//         </form>
//         <p><a href="/register">没有账号？立即注册</a></p>
//     </div>
// </body>
// </html>)";
//     rsp->SetContent(html, "text/html");
// }

// // 简单的用户存储（文件存储）
// class SimpleUserDB {
// private:
//     std::string _db_file = "users.txt";
    
//     std::string HashPassword(const std::string& password) {
//         // 简单的哈希，生产环境应该使用bcrypt等
//         return std::to_string(std::hash<std::string>{}(password));
//     }
    
// public:
//     bool RegisterUser(const std::string& username, const std::string& password, const std::string& email) {
//         std::ofstream file(_db_file, std::ios::app);
//         file << username << ":" << HashPassword(password) << ":" << email << "\n";
//         return true;
//     }
    
//     bool ValidateUser(const std::string& username, const std::string& password) {
//         std::ifstream file(_db_file);
//         std::string line;
//         while (std::getline(file, line)) {
//             size_t pos1 = line.find(':');
//             size_t pos2 = line.find(':', pos1 + 1);
//             if (pos1 != std::string::npos && 
//                 line.substr(0, pos1) == username &&
//                 line.substr(pos1 + 1, pos2 - pos1 - 1) == HashPassword(password)) {
//                 return true;
//             }
//         }
//         return false;
//     }
// };

// SimpleUserDB user_db;

// void ApiRegister(const HttpRequest& req, HttpResponse* rsp) {
//     std::string username = req.GetParam("username");
//     std::string password = req.GetParam("password");
//     std::string email = req.GetParam("email");
    
//     if (username.empty() || password.empty()) {
//         rsp->SetContent(R"({"status": "error", "message": "用户名和密码不能为空"})", "application/json");
//         rsp->_statu = 400;
//         return;
//     }
    
//     if (user_db.RegisterUser(username, password, email)) {
//         rsp->SetContent(R"({"status": "success", "message": "注册成功"})", "application/json");
//     } else {
//         rsp->SetContent(R"({"status": "error", "message": "注册失败"})", "application/json");
//         rsp->_statu = 500;
//     }
// }

// void ApiLogin(const HttpRequest& req, HttpResponse* rsp) {
//     std::string username = req.GetParam("username");
//     std::string password = req.GetParam("password");
    
//     if (user_db.ValidateUser(username, password)) {
//         std::string session_id = GenerateSessionId();
//         user_sessions[session_id] = username;
        
//         // 设置Cookie
//         rsp->SetHeader("Set-Cookie", "session_id=" + session_id + "; Path=/; HttpOnly");
//         rsp->SetContent(R"({"status": "success", "message": "登录成功"})", "application/json");
//     } else {
//         rsp->SetContent(R"({"status": "error", "message": "用户名或密码错误"})", "application/json");
//         rsp->_statu = 401;
//     }
// }

// void Dashboard(const HttpRequest& req, HttpResponse* rsp) {
//     // 检查Session
//     std::string cookie = req.GetHeader("Cookie");
//     std::string session_id;
    
//     // 简单的Cookie解析
//     size_t pos = cookie.find("session_id=");
//     if (pos != std::string::npos) {
//         size_t end = cookie.find(';', pos);
//         session_id = cookie.substr(pos + 11, end - pos - 11);
//     }
    
//     if (session_id.empty() || user_sessions.find(session_id) == user_sessions.end()) {
//         rsp->SetRedirect("/login");
//         return;
//     }
    
//     std::string username = user_sessions[session_id];
//     std::string html = R"(<!DOCTYPE html>
// <html>
// <head>
//     <meta charset="utf-8">
//     <title>用户面板</title>
// </head>
// <body>
//     <h1>欢迎, )" + username + R"(!</h1>
//     <p>这是你的个人面板</p>
//     <a href="/logout">退出登录</a>
// </body>
// </html>)";
//     rsp->SetContent(html, "text/html");
// }

// void Logout(const HttpRequest& req, HttpResponse* rsp) {
//     // 清除Session
//     std::string cookie = req.GetHeader("Cookie");
//     size_t pos = cookie.find("session_id=");
//     if (pos != std::string::npos) {
//         size_t end = cookie.find(';', pos);
//         std::string session_id = cookie.substr(pos + 11, end - pos - 11);
//         user_sessions.erase(session_id);
//     }
    
//     // 清除Cookie
//     rsp->SetHeader("Set-Cookie", "session_id=; expires=Thu, 01 Jan 1970 00:00:00 GMT");
//     rsp->SetRedirect("/login");
// }

// int main() {
//     HttpServer server(8889);
//     server.SetThreadCnt(3);
//     server.SetBasedir("./wwwroot");
    
//     // 页面路由
//     server.Get("/", [](auto& req, auto* rsp) {
//         rsp->SetRedirect("/dashboard");
//     });
//     server.Get("/register", RegisterPage);
//     server.Get("/login", LoginPage);
//     server.Get("/dashboard", Dashboard);
//     server.Get("/logout", Logout);
    
//     // API路由
//     server.Post("/api/register", ApiRegister);
//     server.Post("/api/login", ApiLogin);
    
//     std::cout << "用户系统服务器启动!" << std::endl;
//     server.Listen();
//     return 0;
// }