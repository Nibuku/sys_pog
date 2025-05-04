#include "check.hpp"
#include "common.h"
#include <iostream>
#include <fcntl.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>

using namespace std;

struct user_info {
    string name;
    string hash;
    string uid;
    string home;
    string shell;
    vector<string> groups;
    vector<string> admin_groups;
};

vector<string> split(const string& str, char delimiter) {
    vector<string> result;
    string current_field;
    for (const char& c : str) {
        if (c == delimiter) {
            result.push_back(current_field);
            current_field.clear();
        } else {
            current_field.push_back(c);
        }
    }
    result.push_back(current_field);
    return result;
}

vector<string> read_lines(const char* filename) {
    int fd = check(open(filename, O_RDONLY));

    off_t file_size = check(lseek(fd, 0, SEEK_END));
    check(lseek(fd, 0, SEEK_SET));

    vector<char> buffer(file_size + 1);
    size_t bytes_read = check(read(fd, buffer.data(), file_size));
    close(fd);

    string content(buffer.data());
    return split(content, '\n');
}

void get_user_groups(user_info& user) {
    struct passwd *pw = getpwnam(user.name.c_str());
    if (!pw) return;

    int ngroups = 0;
    getgrouplist(user.name.c_str(), pw->pw_gid, nullptr, &ngroups);
    if (ngroups <= 0) return;

    gid_t *groups = new gid_t[ngroups];
    if (getgrouplist(user.name.c_str(), pw->pw_gid, groups, &ngroups) == -1) {
        delete[] groups;
        return;
    }

    vector<string> group_lines = read_lines("/etc/group");
    for (int i = 0; i < ngroups; ++i) {
        for (const string& line : group_lines) {
            vector<string> fields = split(line, ':');
            if (fields.size() >= 3 && stoi(fields[2]) == groups[i]) {
                user.groups.push_back(fields[0]);

                // Проверяем, является ли пользователь администратором группы
                if (fields.size() >= 4) {
                    vector<string> admins = split(fields[3], ',');
                    for (const string& admin : admins) {
                        if (admin == user.name) {
                            user.admin_groups.push_back(fields[0]);
                            break;
                        }
                    }
                }
                break;
            }
        }
    }
    delete[] groups;
}

vector<user_info> get_passwd() {
    vector<user_info> users;

    uid_t original_uid = getuid();
    uid_t original_euid = geteuid();

    check(seteuid(0));

    vector<string> passwd_lines = read_lines("/etc/passwd");
    vector<string> shadow_lines = read_lines("/etc/shadow");

    check(seteuid(original_euid));

    for (const string& line : passwd_lines) {
        vector<string> fields = split(line, ':');
        if (fields.size() >= 7) {
            user_info user;
            user.name = fields[0];
            user.uid = fields[2];
            user.home = fields[5];
            user.shell = fields[6];

            for (const string& sh : shadow_lines) {
                vector<string> sf = split(sh, ':');
                if (sf.size() >= 2 && sf[0] == user.name) {
                    user.hash = sf[1];
                    break;
                }
            }

            get_user_groups(user);
            users.push_back(user);
        }
    }
    return users;
}

void print_user_info(const user_info& user) {
    cout << "Пользователь: " << user.name << endl;
    cout << "  UID: " << user.uid << endl;
    cout << "  Домашний каталог: " << user.home << endl;
    cout << "  Оболочка: " << user.shell << endl;
    cout << "  Хэш пароля: " << user.hash << endl;

    cout << "  Группы: ";
    for (size_t i = 0; i < user.groups.size(); ++i) {
        cout << user.groups[i];
        // Помечаем группы, где пользователь администратор
        for (const string& admin_group : user.admin_groups) {
            if (admin_group == user.groups[i]) {
                cout << "(admin)";
                break;
            }
        }
        if (i < user.groups.size() - 1) cout << ", ";
    }
    cout << " "<<endl;
}

int main() {

    vector<user_info> users = get_passwd();
    for (const auto& user : users) {
        print_user_info(user);
    }

    return 0;
}