#include <iostream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <iomanip>

namespace fs = std::filesystem;

const std::string STORE_DIR = ".ding_store"; // 存放所有存档的目录
std::string SELF_NAME;                         // 程序自身的文件名，初始化时获取

// 获取存档的具体路径
fs::path get_slot_path(const std::string& slot_name) {
    return fs::current_path() / STORE_DIR / slot_name;
}

// 判断是否需要忽略（不复制、不删除）
bool should_ignore(const fs::path& path) {
    std::string filename = path.filename().string();

    // 1. 忽略存档根目录
    if (filename == STORE_DIR) return true;

    if (filename == SELF_NAME) return true;

    if (filename == ".git") return true;

    return false;
}

// 递归复制目录
void copy_dir(const fs::path& src, const fs::path& dst) {
    // 如果目标不存在，创建
    if (!fs::exists(dst)) {
        fs::create_directories(dst);
    }

    for (const auto& item : fs::directory_iterator(src)) {
        if (should_ignore(item.path())) continue;

        fs::path dest_item = dst / item.path().filename();

        if (fs::is_directory(item.path())) {
            copy_dir(item.path(), dest_item);
        } else {
            // 覆盖复制
            fs::copy_file(item.path(), dest_item, fs::copy_options::overwrite_existing);
        }
    }
}

// 清空当前工作区（除了忽略的文件）
void clean_workspace() {
    for (const auto& item : fs::directory_iterator(fs::current_path())) {
        if (should_ignore(item.path())) continue;

        if (fs::is_directory(item.path())) {
            fs::remove_all(item.path());
        } else {
            fs::remove(item.path());
        }
    }
}

// --- 命令实现 ---

void cmd_save(const std::string& slot_name) {
    if (slot_name.empty() || slot_name.find('/') != std::string::npos) {
        std::cerr << "错误: 存档名不能为空且不能包含 '/'" << std::endl;
        return;
    }

    fs::path target = get_slot_path(slot_name);
    std::cout << "正在保存 [ " << slot_name << " ] ..." << std::endl;

    // 先删除旧存档，确保存档是当前状态的纯净镜像
    if (fs::exists(target)) {
        fs::remove_all(target);
    }

    copy_dir(fs::current_path(), target);
    std::cout << "保存成功！" << std::endl;
}

void cmd_load(const std::string& slot_name) {
    fs::path source = get_slot_path(slot_name);

    if (!fs::exists(source)) {
        std::cerr << "错误: 存档 [ " << slot_name << " ] 不存在。" << std::endl;
        return;
    }

    std::cout << "警告：即将清空当前目录并恢复到 [ " << slot_name << " ]" << std::endl;
    std::cout << "确认继续? "; // 简单的交互，也可以加输入 y/n 确认，这里为了流畅直接回车确认
    std::string confirm;
    std::getline(std::cin, confirm);

    if (confirm != "y" && confirm != "Y") {
        std::cout << "取消操作。" << std::endl;
        return;
    }

    std::cout << "正在恢复..." << std::endl;
    clean_workspace();     // 1. 清理当前
    copy_dir(source, fs::current_path()); // 2. 拷贝回来
    std::cout << "恢复完成！" << std::endl;
}

void cmd_list() {
    fs::path saves_dir = fs::current_path() / STORE_DIR / "saves";

    if (!fs::exists(saves_dir)) {
        std::cout << "当前没有任何存档。" << std::endl;
        return;
    }

    std::cout << std::left << std::setw(20) << "存档名称" << " " << "时间" << std::endl;
    std::cout << std::string(40, '-') << std::endl;

    for (const auto& entry : fs::directory_iterator(saves_dir)) {
        if (entry.is_directory()) {
            auto ftime = fs::last_write_time(entry.path());
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
            std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);

            std::cout << std::left << std::setw(20) << entry.path().filename().string()
            << " " << std::asctime(std::localtime(&cftime));
        }
    }
}

void cmd_delete(const std::string& slot_name) {
    fs::path target = get_slot_path(slot_name);
    if (!fs::exists(target)) {
        std::cerr << "错误: 存档 [ " << slot_name << " ] 不存在。" << std::endl;
        return;
    }

    fs::remove_all(target);
    std::cout << "已删除存档: " << slot_name << std::endl;
}

void print_help(const std::string& prog_name) {
    std::cout << "用法: " << prog_name << " <命令> [参数]" << std::endl;
    std::cout << "Ding! 你的极速版本控制器 " << std::endl;
    std::cout << "\n命令:" << std::endl;
    std::cout << "  save <name>   保存到指定档" << std::endl;
    std::cout << "  load <name>   恢复指定存档" << std::endl;
    std::cout << "  list          查看存档列表" << std::endl;
    std::cout << "  rm <name>     删除指定存档" << std::endl;
    std::cout << "  help          显示帮助信息" << std::endl;
}

int main(int argc, char* argv[]) {
    // 0. 获取程序自身的名字，防止把自己删了或者把自己存进去
    if (argc > 0) {
        SELF_NAME = fs::path(argv[0]).filename().string();
    } else {
        SELF_NAME = "ding";
    }

    if (argc < 2) {
        print_help(SELF_NAME);
        return 1;
    }

    std::string command = argv[1];

    if (command == "save") {
        if (argc < 3) { std::cerr << "请指定存档名称" << std::endl; return 1; }
        cmd_save(argv[2]);
    }
    else if (command == "load") {
        if (argc < 3) { std::cerr << "请指定存档名称" << std::endl; return 1; }
        cmd_load(argv[2]);
    }
    else if (command == "list" || command == "ls") {
        cmd_list();
    }
    else if (command == "rm" || command == "delete") {
        if (argc < 3) { std::cerr << "请指定要删除的存档名称" << std::endl; return 1; }
        cmd_delete(argv[2]);
    }
    else if (command == "help" || command == "--help") {
        print_help(SELF_NAME);
    }
    else {
        std::cerr << "未知命令: " << command << std::endl;
        print_help(SELF_NAME);
        return 1;
    }

    return 0;
}
