#include "check.hpp"
#include <iostream>
#include <fcntl.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>

using namespace std;

class UserInfo {

    struct user_info {
        string name;
        string hash;
        string uid;
        string home;
        vector<string> groups;
        vector<string> admin_groups;
    };

    vector<user_info*> _users;

    void clear_users() {
        for (auto *user: _users) {
            delete user;
        }
        _users.clear();
    }

    vector<string> split(const string &str, char delimiter) {
        vector<string> result;
        string current;
        for (char c: str) {
            if (c == delimiter) {
                result.push_back(current);
                current.clear();
            } else {
                current.push_back(c);
            }
        }
        result.push_back(current);
        return result;
    }

    vector<string> read_lines(int fd) {

        off_t file_size = check(lseek(fd, 0, SEEK_END));
        check(lseek(fd, 0, SEEK_SET));

        vector<char> buffer(file_size + 1);
        check(read(fd, buffer.data(), file_size));
        close(fd);

        string content(buffer.data());
        return split(content, '\n');
    }

    void get_user_groups(user_info &user, const vector<string> &group_lines, const vector<string> &gshadow_lines) {
//        struct passwd *pw = getpwnam(user.name.c_str());
//        if (!pw)
//            return;
//
//        int ngroups;
//        getgrouplist(user.name.c_str(), pw->pw_gid, nullptr, &ngroups);
//
//        vector<gid_t> groups(ngroups);
//        getgrouplist(user.name.c_str(), pw->pw_gid, groups.data(), &ngroups);
//
//        for (const string &line: group_lines) {
//            vector<string> fields = split(line, ':');
//            if (fields.size() < 3)
//                continue;
//
//            gid_t gid = stoi(fields[2]);
//
//            for (gid_t g: groups) {
//                if (g == gid) {
//                    user.groups.push_back(fields[0]);
//                    break;
//                }
//            }
//        }
        for (const string &line: group_lines) {
            vector<string> fields = split(line, ':');
            if (fields.size() < 3)
                continue;

            const string &group_name = fields[0];
            const string &admin_field = fields[3];
            vector<string> members = split(admin_field, ',');

            for (const string &member: members) {
                if (member == user.name) {
                    user.groups.push_back(group_name);
                    break;
                }
            }
            if (user.groups.empty())
                user.groups.push_back(user.name);
        }
        for (const string &line: gshadow_lines) {
            vector<string> fields = split(line, ':');
            if (fields.size() < 4)
                continue;

            const string &group_name = fields[0];
            const string &admin_field = fields[2];
            vector<string> admins = split(admin_field, ',');

            for (const string &admin: admins) {
                if (admin == user.name) {
                    user.admin_groups.push_back(group_name);
                    break;
                }
            }
        }
    }

    void get_passwd() {

        uid_t original_euid = geteuid();

        int fd_shadow = check(open("/etc/shadow", O_RDONLY));
        int fd_gshadow = check(open("/etc/gshadow", O_RDONLY));

        check(seteuid(original_euid));

        int fd_passwd = check(open("/etc/passwd", O_RDONLY));
        int fd_group = check(open("/etc/group", O_RDONLY));


        vector<string> shadow_lines = read_lines(fd_shadow);
        vector<string> gshadow_lines = read_lines(fd_gshadow);
        vector<string> passwd_lines = read_lines(fd_passwd);
        vector<string> group_lines = read_lines(fd_group);


        for (const string &line: passwd_lines) {
            vector<string> fields = split(line, ':');
            if (fields.size() >= 7) {
                auto *user = new user_info;
                user->name = fields[0];
                user->uid = fields[2];
                user->home = fields[5];

                for (const string &sh: shadow_lines) {
                    vector<string> sf = split(sh, ':');
                    if (sf.size() >= 2 && sf[0] == user->name) {
                        user->hash = sf[1];
                        break;
                    }
                }
                get_user_groups(*user, group_lines, gshadow_lines);
                _users.push_back(user);
            }
        }
    }

public:

    UserInfo() = default;

    ~UserInfo() {
        clear_users();
    }

    void get_user_info() {
        get_passwd();
    }

    void print_user_info() {
        for (const auto *user: _users) {
            cout << "Пользователь: " << user->name << endl;
            cout << " UID: " << user->uid << endl;
            cout << " Домашний каталог: " << user->home << endl;
            cout << " Хэш пароля: " << user->hash << endl;

            cout << " Группы: ";
            for (size_t i = 0; i < user->groups.size(); ++i) {
                cout << user->groups[i];
                if (i != user->groups.size() - 1)
                    cout << ", ";
            }
            cout << endl;

            cout << " Администратор в группах: ";
            for (size_t i = 0; i < user->admin_groups.size(); ++i) {
                cout << user->admin_groups[i];
                if (i != user->admin_groups.size() - 1)
                    cout << ", ";
            }
            cout << endl << endl;
        }
    }
};

int main() {
    UserInfo info;
    info.get_user_info();
    info.print_user_info();
    return 0;
}
