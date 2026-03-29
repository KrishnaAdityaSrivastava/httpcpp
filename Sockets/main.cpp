#include "listenSocket.hpp"
#include <iostream>

using namespace std;

int main(){
    cout << "Starting"<<endl;

    cout << "Binding Socket"<<endl;
  HTTP::BindSocket bs = HTTP::BindSocket(AF_INET, SOCK_STREAM,0,80,INADDR_ANY);

    cout << "Listening"<<endl;
    HTTP::ListenSocket ls = HTTP::ListenSocket(AF_INET, SOCK_STREAM,0,81,INADDR_ANY,10);

    cout <<"Success";
}