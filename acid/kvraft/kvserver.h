//
// Created by zavier on 2022/12/3.
//

#ifndef ACID_KVSERVER_H
#define ACID_KVSERVER_H

#include "../raft/raft_node.h"
#include "commom.h"

namespace acid::kvraft {
using namespace acid::raft;
class KVServer {
public:
    using ptr = std::shared_ptr<KVServer>;
    using MutexType = co::co_mutex;
    using KVMap = std::map<std::string, std::string>;

    /* maxRaftState：一个阈值，超过之后 KVServer 会生成快照替换日志 */
    KVServer(std::map<int64_t, std::string>& servers, int64_t id, Persister::ptr persister, int64_t maxRaftState = 1000);
    ~KVServer();
    
    /* 通过 m_persister 从本地加载一个最近的快照，并调用 readSnapshot 来恢复之前的状态， 
       然后启动一个协程执行 applier 函数，最后启动 raft 服务器并阻塞在这里 */
    void start();
    void stop();
    CommandResponse handleCommand(CommandRequest request);
    CommandResponse Get(const std::string& key);
    CommandResponse Put(const std::string& key, const std::string& value);
    CommandResponse Append(const std::string& key, const std::string& value);
    CommandResponse Delete(const std::string& key);
    CommandResponse Clear();
    /* C++17 当用于描述函数的返回值时，如果调用函数的地方没有获取返回值时，编译器会给予警告 */
    [[nodiscard]]
    const KVMap& getData() const { return m_data;}
private:
    /* 不断从 m_applychan 里接收 raft 达成共识的消息，再根据消息类型进行对应的操作 */
    void applier();
    void saveSnapshot(int64_t index);
    /* 读m_persister中最近的一个快照 */
    void readSnapshot(Snapshot::ptr snap);
    bool isDuplicateRequest(int64_t client, int64_t command);
    bool needSnapshot();
    CommandResponse applyLogToStateMachine(const CommandRequest& request);
private:
    MutexType m_mutex;
    int64_t m_id;                            // raft node 的 id
    co::co_chan<raft::ApplyMsg> m_applychan; // 接收 raft 达成共识消息的 channel

    KVMap m_data;                            // map实现的键值存储
    Persister::ptr m_persister;              // raft持久化模块
    std::unique_ptr<RaftNode> m_raft;        // 指向一个raft服务器的智能指针

    std::map<int64_t, std::pair<int64_t, CommandResponse>> m_lastOperation;
    std::map<int64_t, co::co_chan<CommandResponse>> m_nofiyChans;

    int64_t m_lastApplied = 0;
    int64_t m_maxRaftState = -1;
};
}
#endif //ACID_KVSERVER_H
