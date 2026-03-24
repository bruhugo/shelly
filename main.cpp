#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>

#include <cstdlib>
#include <string>
#include <vector>
#include <stdexcept>
#include <filesystem>
#include <iostream>

using namespace std;
using token = string;

string WDIR = "~/";

struct CMDStream {
    string filepath;
    bool override;
    bool assigned;
    int fd;

    CMDStream(){}

    CMDStream(int fd){
        this->fd = fd;
        override = false;
        assigned = false;
    }
};

struct CMD {
    string name;
    vector<string> args;
    CMDStream stderr;
    CMDStream stdout;

    int stdin;

    CMD(){
        stderr = CMDStream(2);
        stdout = CMDStream(1); 
    }
    
    bool assignCommandStream(const string& cmdStream, const string& filepath) {
        CMDStream* target = nullptr;
        bool isAppend = false;

        if (cmdStream == "e>" || cmdStream == "e>>") {
            target = &stderr;
            isAppend = cmdStream == "e>>";
        } else if (cmdStream == "o>" || cmdStream == "o>>") {
            target = &stdout;
            isAppend = cmdStream == "o>>";
        } else {
            return false; 
        }

        if (filepath.empty())
            throw runtime_error("A filepath must be specified for redirecting streams.");

        if (target->assigned)
            throw runtime_error("Stream already assigned.");

        target->assigned = true;
        target->filepath = filepath;
        target->override = !isAppend;
        return true;
    }
};

bool isSpace(char c);

vector<token> getTokens(string cmd);
vector<token> getTokens(string cmd);
vector<CMD> getCommands(const vector<token>& tokens);
void executeCommands(const vector<CMD>& commands);
bool handleCD(const vector<token>& tokens);

int main(){

    if(char *home = getenv("HOME")){
        WDIR = string(home);
    }else{
        WDIR = "/";
    }

    while (true){
        cout << "shelly " << WDIR << " "; 
        string cmd;
        getline(cin, cmd);

        if (cmd.compare("exit") == 0) return 0;
       
        try{
            vector<token> tokens = getTokens(cmd);
            if (handleCD(tokens)) continue;

            vector<CMD> commands = getCommands(tokens);
            executeCommands(commands);
        }catch(const runtime_error& err){
            cout << err.what() << endl;
        }
    }

    return 0;
}

bool isSpace(char c){
    return c == ' ' || c == '\n' || c == '\t';
}

vector<token> getTokens(string cmd){
    vector<token> tokens;
    for (auto it = cmd.begin();;){
        while (isSpace(*it))
            ++it;

        if (it == cmd.end()) break;

        token t;
        bool quote = *it == '"';
        if (quote){
            t += *it; ++it;
        }
        while (it != cmd.end() && (!isSpace(*it) || quote)){
            t += *it;
            if (*it == '"')
                quote = false;
            ++it;
        }
        tokens.push_back(t);
        
        if (it == cmd.end()) break;
    }

    return tokens;
}

vector<CMD> getCommands(const vector<token>& tokens){
    vector<CMD> commands;
    for (auto it = tokens.begin();;){
        CMD cmd;

        cmd.name = *it; ++it;
        for (;it != tokens.end() && *it != "|" ; ++it){
            string filepath;
            if (it + 1 != tokens.end()){
                filepath = *(it + 1);
            }
            
            if (cmd.assignCommandStream(*it, filepath)){
                ++it;
                continue; 
            }

            cmd.args.push_back(*it);
        }
        commands.push_back(cmd);
        if (it == tokens.end()) break;

        ++it;
    }
    return commands;
}

void executeCommands(const vector<CMD>& commands) {
    int n = commands.size();
    vector<CMD> cmds(commands); 
    vector<pid_t> pids(n);
    vector<int> pipeFds;

    for (int i = 0; i < n - 1; ++i) {
        int fd[2];
        if (pipe(fd) == -1) {
            perror("pipe");
            return;
        }
        cmds[i].stdout.fd = fd[1];  
        cmds[i + 1].stdin   = fd[0]; 
        pipeFds.push_back(fd[0]);
        pipeFds.push_back(fd[1]);
    }

    for (int i = 0; i < n; ++i) {
        if (cmds[i].stderr.assigned) {
            int fd = open((WDIR + "/" + cmds[i].stderr.filepath).c_str(),
                          O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) { perror("open stderr"); return; }
            cmds[i].stderr.fd = fd;
        }
        if (cmds[i].stdout.assigned) {
            int fd = open((WDIR + "/" + cmds[i].stdout.filepath).c_str(),
                          O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) { perror("open stdout"); return; }
            cmds[i].stdout.fd = fd;
        }
    }

    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            throw runtime_error("Failed to fork process.");
        } else if (pid == 0) {
            for (int pfd : pipeFds) {
                if (pfd != cmds[i].stdin && pfd != cmds[i].stdout.fd)
                    close(pfd);
            }

            chdir(WDIR.c_str());
            dup2(cmds[i].stdin,     0);
            dup2(cmds[i].stdout.fd, 1);
            dup2(cmds[i].stderr.fd, 2);

            if (cmds[i].stdin != STDIN_FILENO)   close(cmds[i].stdin);
            if (cmds[i].stdout.fd != STDOUT_FILENO) close(cmds[i].stdout.fd);
            if (cmds[i].stderr.fd != STDERR_FILENO) close(cmds[i].stderr.fd);

        vector<char*> argv;
        argv.push_back(const_cast<char*>(cmds[i].name.c_str()));
        for (const string& arg : cmds[i].args)
            argv.push_back(const_cast<char*>(arg.c_str()));
        argv.push_back(nullptr);

        execvp(cmds[i].name.c_str(), argv.data());

            execvp(cmds[i].name.c_str(), argv.data());
            perror("execvp"); 
            exit(1);
        }

        pids[i] = pid;
    }

    for (int pfd : pipeFds)
        close(pfd);

    for (int i = 0; i < n; ++i) {
        int status;
        waitpid(pids[i], &status, 0);
    }
}

bool handleCD(const vector<token>& tokens){
    if (tokens.at(0) != "cd") return false;

    if (tokens.size() > 2) 
        throw runtime_error("Too many arguments.");

    token file = tokens.at(1);

    if (file == ".."){
        while(*(WDIR.end() - 1) != '/')
            WDIR.pop_back();
            
        if (WDIR.size() > 1)
            WDIR.pop_back();

        file = WDIR;
    }else if (file.at(0) != '/'){
        if (WDIR.size() > 1){
            file = WDIR + "/" + file;
        }else {
            file = WDIR + file;
        }
    }

    if (!filesystem::exists(file))
        throw runtime_error("No such file or directory.");
    
    WDIR = file;

    return true;
}