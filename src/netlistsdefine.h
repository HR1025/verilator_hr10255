#pragma once

#include <string>
#include <vector>
#include <unordered_map>

/** @brief 错误类型 */
enum class ErrorType { UNKNOWN = -1, FALSE, TRUE };

/** @brief 端口类型
 *  @sa    HDL RTL 级别的 moudle 设计
 */
enum class PortType {
    UNKNOWN = -1,
    INPUT,  // 输入
    OUTPUT,  // 输出
    INOUT,  // 输入输出
    WIRE,  // 连线
    TYPENUM  // 类型种数
};

/**
 * @brief 端口的信息
 * @sa    PortType
 */
struct PortMsg {
    std::string portDefName;  // 端口定义名称
    PortType portType = PortType::UNKNOWN;  // 端口类型
    bool isArray = false;  // 是否是数组
    uint32_t arraySize = 1;  // 数组大小
};

/**
 * @brief 端口实例形参信息
 * @note  1 - indexRange : uint32_t -> start， uint32_t -> len
 */
struct PortInstanceFormalMsg {
    /**
     * @brief portInstanceName == "anonymous" 时即为常量赋值，也就是匿名赋值
     */
    std::string portInstanceName = "anonymous";  // 端口实例名称 (实参)
    bool isArray = false;  // 是否是数组类型
    std::pair<uint32_t, uint32_t> indexRange;  // 引脚范围
    /**
     * @brief 获取索引
     * @note  仅当 isArray == true 时可以调用
     */
    std::vector<int> getIndexs();
};

/** @brief 端口实例信息
 *  @note  只需要知道端口的形参以及实参即可
 * */
struct PortInstanceMsg {
    std::string portDefName;  // 端口定义名称 (形参)
    std::vector<PortInstanceFormalMsg>
        portInstanceFormalMsgs;  // 端口实例组 (实参,参考 c++ 初始化列表)
};

struct AssignStatementMsg {
    PortInstanceFormalMsg lValue;  // 左值
    PortInstanceFormalMsg rValue;  // 右值
};

/**
 * @brief 模块信息
 * @note  需要知道的信息包括以下 ：
 *        1 - 模块自己的名称
 *        2 - 使用的子模块的名称以及其实例名称 (需要知道其顺序)
 *        3 - 需要知道使用到的子模块的引脚及其实例 (由于传入本身就不保证顺序，所以这里不保证)\
 *        4 - 模块的引脚信息
 */
struct MoudleMsg {
public:
    // std::string -> subMoudleInstanceName, std::string -> subModuleDefName
    using MouldeDefInstanceMap = std::unordered_map<std::string, std::string>;
    // (std::string -> subMoudleInstanceName) ， std::vector<PortInstanceMsg> -> 实例引脚表
    using SubMoudlePorts = std::unordered_map<std::string, std::vector<PortInstanceMsg>>;

public:
    std::string moduleDefName;  // 模块定义名称   (形参)
    uint32_t level = 0;  // 模块的层级 (用于排序加速,目前其实并没用)

    /*********************************** 网表定义信息(START)
     * *********************************************/
    std::vector<PortMsg> inputs;  // 输入引脚
    std::vector<PortMsg> outputs;  // 输出引脚
    std::vector<PortMsg> inouts;  // 输入输出引脚 (暂时未发现，作为保留字段)
    std::vector<PortMsg> wires;  // 连线引脚
    std::vector<AssignStatementMsg> assigns;  // assign 语句
    /*********************************** 网表定义信息(END)
     * *********************************************/

    /*********************************** 网表实例信息(START)
     * *********************************************/
    std::vector<std::string> subMoudleInstanceNames;  // 子模块实例名称  (实参)
    MouldeDefInstanceMap mouldeDefInstanceMap;  // 子模块实例映射表
    SubMoudlePorts subMoudlePorts;  // 子模块实例的引脚表
    /*********************************** 网表实例信息(END)
     * *********************************************/

public:
    uint32_t inNum(bool onlyIn = false) const;
    uint32_t outNum(bool onlyOut = false) const;
    uint32_t inoutNum() const;
    uint32_t wireNum() const;
};
