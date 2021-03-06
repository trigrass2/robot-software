#pragma once

#include <error/error.h>
#include <cstdlib>
#include <algorithm>

namespace raft {

const auto HEARTBEAT_PERIOD = 10;
const auto ELECTION_TIMEOUT_MAX = 500;
const auto ELECTION_TIMEOUT_MIN = 100;

// TODO: How many entries ?
const auto LOG_SIZE = 10u;

using NodeId = int;
using Term = int;
using Index = int;

enum class NodeState {
    Follower = 0,
    Candidate,
    Leader
};

template <typename Operation>
struct LogEntry {
    Operation operation;
    Term term;
    Index index;
};

template <typename Operation, int N>
class Log {
    int m_size;
    LogEntry<Operation> entries[N];

    void remove_conflicting_entries(const LogEntry<Operation>* new_entries, int entry_count)
    {
        for (auto i = 0; i < size(); i++) {
            for (auto j = 0; j < entry_count; j++) {
                if (new_entries[j].index == entries[i].index && entries[i].term < new_entries[j].term) {
                    keep_until(i);
                    return;
                }
            }
        }
    }

public:
    Log()
        : m_size(0)
    {
    }

    int size() const
    {
        return m_size;
    }

    void append(LogEntry<Operation> entry)
    {
        if (m_size < N) {
            entries[m_size] = entry;
            m_size++;
        } else {
            // This should not happen if log compaction is ran often enough
            // Note: This error does not threaten the consistency of the log,
            // the request will be dropped silently
            WARNING("log is already full");
        }
    }

    LogEntry<Operation>& operator[](int i)
    {
        return entries[i];
    }

    Index last_index() const
    {
        if (m_size > 0) {
            return entries[m_size - 1].index;
        }

        return 0;
    }

    Term last_term() const
    {
        if (m_size > 0) {
            return entries[m_size - 1].term;
        }

        return 0;
    }

    LogEntry<Operation>* find_entry(Term term, Index index)
    {
        for (auto i = 0; i < size(); i++) {
            if (entries[i].index == index && entries[i].term == term) {
                return &entries[i];
            }
        }

        return nullptr;
    }

    void merge(const LogEntry<Operation>* entries, int entry_count)
    {
        remove_conflicting_entries(entries, entry_count);

        for (auto i = 0; i < entry_count; i++) {
            if (entries[i].index > last_index()) {
                append(entries[i]);
            }
        }
    }

    // Discards all elements after the nth
    void keep_until(int n)
    {
        m_size = n;
    }
};

template <typename StateMachine>
struct Message {
    enum class Type {
        VoteRequest = 0,
        VoteReply,
        AppendEntriesRequest,
        AppendEntriesReply,
    };

    Type type;
    Term term;
    NodeId from_id;

    union {
        struct {
            Index last_log_index;
            Term last_log_term;
        } vote_request;

        struct {
            bool vote_granted;
        } vote_reply;

        struct {
            int count;
            Index leader_commit;
            Term previous_entry_term;
            Index previous_entry_index;
            LogEntry<typename StateMachine::Operation> entries[LOG_SIZE];
        } append_entries_request;

        struct {
            bool success;
            Index last_index;
        } append_entries_reply;
    };

    Message()
    {
        // Set every field to zero
        std::memset(this, 0, sizeof(*this));
    }
};

template <typename StateMachine>
class Peer {
public:
    Peer(NodeId id)
        : id(id)
    {
    }
    virtual void send(const Message<StateMachine>& msg) = 0;
    virtual ~Peer(){};

    NodeId id;

    // Those are per-peer information needed by the raft leader to send correct
    // versions of the log to each peer
    Index match_index;
    Index next_index;
};

template <typename StateMachine>
class State {
public:
    using Message = raft::Message<StateMachine>;
    using Peer = raft::Peer<StateMachine>;
    NodeId id;
    Peer** peers;
    int peer_count;
    Term term;
    int vote_count;
    NodeId voted_for;
    NodeState node_state;
    int heartbeat_timer;
    int election_timer;
    Log<typename StateMachine::Operation, LOG_SIZE> log;
    Index commit_index;

    StateMachine& state_machine;

    State(StateMachine& state_machine, NodeId id, Peer** peers, int peer_count)
        : id(id)
        , peers(peers)
        , peer_count(peer_count)
        , term(0)
        , voted_for(0)
        , node_state(NodeState::Follower)
        , heartbeat_timer(0)
        , election_timer(ELECTION_TIMEOUT_MAX)
        , log()
        , commit_index(0)
        , state_machine(state_machine)
    {
        for (auto i = 0; i < peer_count; i++) {
            peers[i]->match_index = 0;
        }
    }

    bool process(const Message& msg, Message& reply)
    {
        reply.from_id = id;

        switch (msg.type) {
            case Message::Type::VoteRequest: {
                DEBUG("Got a VoteRequest from %d", msg.from_id);
                reply.type = Message::Type::VoteReply;
                reply.vote_reply.vote_granted = false;

                const auto req_lli = msg.vote_request.last_log_index;
                const auto req_llt = msg.vote_request.last_log_term;
                auto valid_candidate = msg.term > term && log.last_index() <= req_lli && log.last_term() <= req_llt;
                auto same_candidate = msg.term == term && msg.from_id == voted_for;
                if (valid_candidate || same_candidate) {
                    reply.vote_reply.vote_granted = true;
                    term = msg.term;
                    voted_for = msg.from_id;
                    node_state = NodeState::Follower;

                    DEBUG("Granted my vote to %d which has term %d", voted_for, term);
                }

                reply.term = msg.term;
                return true;
            }

            case Message::Type::VoteReply: {
                DEBUG("Got a VoteReply(granted = %d)", msg.vote_reply.vote_granted);
                // TODO: How about potential duplicate votes =

                if (node_state != NodeState::Candidate) {
                    break;
                }

                if (msg.vote_reply.vote_granted) {
                    vote_count++;
                    // Reminder: total number of votes = vote_count + 1 because
                    // we also voted for ourselves
                    if (2 * vote_count >= peer_count) {
                        become_leader();
                    }
                } else if (msg.term > term) {
                    term = msg.term;
                    node_state = NodeState::Follower;
                    voted_for = 0;
                    reset_election_timer();
                }

                break;
            }

            case Message::Type::AppendEntriesRequest: {
                reset_election_timer();

                if (msg.term > term) {
                    node_state = NodeState::Follower;
                    term = msg.term;
                }

                reply.type = Message::Type::AppendEntriesReply;

                // If the request comes from an older term, discard itj
                if (msg.term < term) {
                    // TODO: add my own term
                    reply.append_entries_reply.success = false;
                    return true;
                }

                // If the entry described as previous entry in the message does
                // not exist, discard this request
                auto prev_index = msg.append_entries_request.previous_entry_index;
                auto prev_term = msg.append_entries_request.previous_entry_term;
                if (prev_index > 0 && prev_term > 0) {
                    if (!log.find_entry(prev_term, prev_index)) {
                        reply.append_entries_reply.success = false;
                        return true;
                    }
                }

                // Append the new entries to the log
                log.merge(msg.append_entries_request.entries,
                          msg.append_entries_request.count);

                // Update the commit value
                auto leader_commit = msg.append_entries_request.leader_commit;
                if (leader_commit > commit_index) {
                    auto new_index = std::min(leader_commit, log.last_index());
                    commit_log_entries(commit_index, new_index);
                    commit_index = new_index;
                }

                // Checks for conflicting entries, meaning entries that have
                // the same index as one in the log, but not with the same
                // term, then solves this conflict.

                reply.append_entries_reply.success = true;
                reply.append_entries_reply.last_index = log.last_index();

                return true;
            }

            case Message::Type::AppendEntriesReply: {
                if (msg.append_entries_reply.success) {
                    auto last_index = msg.append_entries_reply.last_index;
                    for (auto i = 0; i < peer_count; i++) {
                        if (peers[i]->id == msg.from_id) {
                            peers[i]->match_index = last_index;
                            peers[i]->next_index = last_index + 1;
                        }
                    }
                    auto new_index = find_safe_index();
                    commit_log_entries(commit_index, new_index);
                    commit_index = new_index;
                } else {
                    for (auto i = 0; i < peer_count; i++) {
                        if (peers[i]->id == msg.from_id) {
                            peers[i]->next_index--;
                        }
                    }
                }
                break;
            }
        }

        return false;
    }

    void start_election()
    {
        if (node_state == NodeState::Leader) {
            return;
        }

        NOTICE("Starting election...");
        node_state = NodeState::Candidate;
        term++;
        vote_count = 0;
        voted_for = id;

        Message msg;
        msg.type = Message::Type::VoteRequest;
        msg.term = term;
        msg.from_id = id;

        msg.vote_request.last_log_index = log.last_index();
        msg.vote_request.last_log_term = log.last_term();

        for (auto i = 0; i < peer_count; i++) {
            peers[i]->send(msg);
        }
    }

    void tick()
    {
        if (node_state == NodeState::Leader) {
            tick_heartbeat();
        } else {
            tick_election();
        }
    }

    void replicate(typename StateMachine::Operation operation)
    {
        // TODO: If we are not a leader we should forward this to the leader
        LogEntry<typename StateMachine::Operation> entry;
        entry.operation = operation;
        entry.term = term;
        entry.index = log.last_index() + 1;
        log.append(entry);
    }

    void become_leader()
    {
        node_state = NodeState::Leader;
        for (auto i = 0; i < peer_count; i++) {
            peers[i]->next_index = log.last_index();
            peers[i]->match_index = 0;
        }
    }

private:
    void tick_heartbeat()
    {
        if (heartbeat_timer > 0) {
            heartbeat_timer--;
        } else {
            DEBUG("Sending heartbeat");
            for (auto i = 0; i < peer_count; i++) {
                Message msg;
                msg.type = Message::Type::AppendEntriesRequest;
                msg.from_id = id;
                msg.term = term;
                msg.append_entries_request.count = 0;
                msg.append_entries_request.leader_commit = commit_index;

                auto peer = peers[i];
                auto next_index = peer->next_index;

                // Copy all the log with a greater index than the last one for
                // this peer
                for (auto j = 0; j < log.size(); j++) {
                    if (log[j].index >= next_index) {
                        for (auto k = 0; k + j < log.size(); k++) {
                            msg.append_entries_request.entries[k] = log[k + j];
                            msg.append_entries_request.count++;
                        }
                        break;
                    }
                    msg.append_entries_request.previous_entry_term = log[j].term;
                    msg.append_entries_request.previous_entry_index = log[j].index;
                }

                peer->send(msg);
            }

            // Rearm timer
            heartbeat_timer = HEARTBEAT_PERIOD - 1;
        }
    }

    void tick_election()
    {
        if (election_timer > 0) {
            election_timer--;
        } else {
            start_election();
            reset_election_timer();
        }
    }

    void reset_election_timer()
    {
        auto x = ELECTION_TIMEOUT_MAX - ELECTION_TIMEOUT_MIN;

        election_timer = ELECTION_TIMEOUT_MIN;
        election_timer += std::rand() % x;
    }

    Index find_safe_index()
    {
        // Finds an index N such that a majority of match_index >= N and log[N]
        // == currentTerm
        // See sections 5.3 and 5.4 of the raft paper for why those properties
        // are important

        // Step 1 sort the peers according to their match index.
        // This allows us to take N as the median value of the match index,
        // ensuring that a majority of nodes have a match_index of at least N
        std::qsort(peers, peer_count, sizeof(peers[0]), [](const void* a, const void* b) {
            Peer* pa = (Peer*)a;
            Peer* pb = (Peer*)b;
            if (pa->match_index < pb->match_index) {
                return 1;
            } else if (pa->match_index > pb->match_index) {
                return -1;
            }
            return 0;
        });

        const auto median_peer = std::max(peer_count / 2 - 1, 0);
        auto N = peers[median_peer]->match_index;

        // Then check that the entry with index N is from the current term.
        // This is important to ensure consistency when the leader changed
        // recently
        for (auto i = 0; i < log.size(); i++) {
            if (log[i].index == N && log[i].term == term) {
                return N;
            }
        }

        // If we are not able to find a safe N to commit, then return the
        // current commit index.
        return commit_index;
    }

    void commit_log_entries(Index old_index, Index new_index)
    {
        auto i = 0;

        // First find all non commited entries
        auto commit_index = old_index;
        for (; i < log.size() && log[i].index <= commit_index; i++) {
        }

        // Then commit all new entries
        for (; commit_index < new_index; commit_index++, i++) {
            state_machine.apply(log[i].operation);
        }
    }
};
} // namespace raft
