#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>
#include <chrono>
#include <iomanip>

using namespace std;

string trim(string input)
{

    string final_input;

    while(input[0] == ' '){
        input = input.erase(0, 1);
    }
    while(input.back() == ' '){
        input.pop_back();
    }
    return input;
}

char **vec_to_char_array(vector<string> &parts)
{
    char **charray = new char *[parts.size() + 1];
    for (int i = 0; i < parts.size(); i++)
    {
        charray[i] = (char *)parts[i].c_str();
    }

    charray[parts.size()] = NULL;
    return charray;
}

vector<string> split(string line, char spacer = ' ')
{

    vector<string> new_input;
    int integer = 0;
    int quote_check = 0;
    bool insiders = false;
    int start = 0;

    for (int i = 0; i < line.size(); i++)
    {

        if (spacer == ' ' and (line[i] == '\'' or line[i] == '\"'))
        {

            string thing;
            quote_check++;
            if (quote_check % 2 == 0)
            {

                insiders = false;
                for (int j = start; j < i; j++)
                    thing.push_back(line[j]);
                new_input.push_back(thing);
            }
            else
            {
                insiders = true;
                start = i + 1;
            }
        }
        else if (!insiders && (line[i] == spacer) || (i == (line.size() - 1)))
        {
            string more_things;
            if (i != line.size() - 1)
            {
                for (int j = integer; j < i; j++)
                    more_things.push_back(line[j]);
            }
            else
            {
                for (int j = integer; j < i + 1; j++)
                    more_things.push_back(line[j]);
            }
            integer = i + 1;

            new_input.push_back(more_things);
        }
    }
    return new_input;
}

int main()
{
    vector<int> processes;
    int first_input = dup(0);
    int other_input = dup(1);

    char buffer[1000];

    string directory = getcwd(buffer, sizeof(buffer));
    chdir("..");
    string prev_directory = getcwd(buffer, sizeof(buffer));
    chdir(directory.c_str());

    while (true)
    {
        bool process = false;
        dup2(first_input, 0);
        dup2(other_input, 1);
        for (int i = 0; i < processes.size(); i++) {

            if (waitpid(processes[i], 0, WNOHANG) == processes[i]){

                cout << "The process: " << processes[i] << " has finished" << endl;
                processes.erase(processes.begin() + i);
                i--;
            }
        }


        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        std::stringstream ss;
        ss << put_time(std::localtime(&in_time_t), "%a %b %d %H:%M:%S %Z %Y");
        cout << "priyanshub " << ss.str() << endl;
        string line_input;
        getline(cin, line_input);

        if (line_input == string("exit"))
        {
            cout << "Bye!! End of Shell " << endl;
            break;
        }

        line_input = trim(line_input);

        if (line_input[line_input.size() - 1] == '&')
        {

            cout << "There is a process running!" << endl;
            process = true;
            line_input = trim(line_input.substr(0, line_input.size() - 1));
        }

        vector<string> newvector = split(line_input, '|');

        for (int i = 0; i < newvector.size(); i++)
        {

            int fds[2];
            pipe(fds);
            line_input = trim(newvector[i]);

            int pid = fork();
            int position = 0;
            if (pid == 0)
            {

                if (trim(line_input).find("cd") == 0)
                {
                    directory = getcwd(buffer, sizeof(buffer));
                    if (trim(line_input).find("-") == 3)
                    {
                        chdir(prev_directory.c_str());
                        prev_directory = directory;
                    }
                    else
                    {
                        string directory_name = trim(split(line_input)[1]);
                        chdir(directory_name.c_str());
                        prev_directory = directory;
                    }
                    continue;
                }
                while (position != string::npos)
                {

                    position = line_input.find('>');
                    if (position >= 0)
                    {
                        string filename = trim(line_input.substr(position + 1));
                        string request = trim(line_input.substr(0, position));
                        line_input = request;
                        int fd = open(filename.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                        dup2(fd, 1);
                        close(fd);
                    }
                    position = line_input.find('<');
                    if (position >= 0)
                    {
                        string filename = trim(line_input.substr(position + 1));
                        string request = trim(line_input.substr(0, position));
                        line_input = request;
                        int fd = open(filename.c_str(), O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                        dup2(fd, 0);
                        close(fd);
                    }
                    if (i < newvector.size() - 1)
                    {
                        dup2(fds[1], 1);
                        close(fds[1]);
                    }

                    vector<string> parts = split(line_input);
                    char **args = vec_to_char_array(parts);
                    execvp(args[0], args);
                }
            }
            else
            {
                if (process)
                {
                    processes.push_back(pid);
                }
                else
                {
                    waitpid(pid, 0, 0);
                }

                dup2(fds[0], 0);
                close(fds[1]);
            }
        }
    }
    return 0;
}