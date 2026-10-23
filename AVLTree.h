#pragma once
#include "Job.h"
#include <algorithm>
#include <memory>
#include <ostream>
#include <vector>
using namespace std;

// AVL tree: nodes are keyed by job ID and hold the shared Job object.
class AVLTree {
private:
    struct Node {
        int key;
        shared_ptr<Job> job;
        int height{1};
        unique_ptr<Node> left, right;
        explicit Node(shared_ptr<Job> j) : key(j->id), job(std::move(j)) {}
    };
    unique_ptr<Node> root;
    size_t recordCount{0};
    long long insertions{0}, deletions{0}, leftRotations{0}, rightRotations{0};

    static int h(const unique_ptr<Node>& n) { return n ? n->height : 0; }
    static void updateHeight(Node* n) { n->height = 1 + max(h(n->left), h(n->right)); }
    static int balance(const Node* n) { return h(n->left) - h(n->right); }

    unique_ptr<Node> rotateRight(unique_ptr<Node> y) {
        auto x = std::move(y->left);
        auto middle = std::move(x->right);
        x->right = std::move(y);
        x->right->left = std::move(middle);
        updateHeight(x->right.get());
        updateHeight(x.get());
        ++rightRotations;
        return x;
    }
    unique_ptr<Node> rotateLeft(unique_ptr<Node> x) {
        auto y = std::move(x->right);
        auto middle = std::move(y->left);
        y->left = std::move(x);
        y->left->right = std::move(middle);
        updateHeight(y->left.get());
        updateHeight(y.get());
        ++leftRotations;
        return y;
    }
    unique_ptr<Node> rebalance(unique_ptr<Node> n) {
        if (!n) return n;
        updateHeight(n.get());
        int bf = balance(n.get());
        if (bf > 1) {
            if (balance(n->left.get()) < 0) n->left = rotateLeft(std::move(n->left));
            return rotateRight(std::move(n));
        }
        if (bf < -1) {
            if (balance(n->right.get()) > 0) n->right = rotateRight(std::move(n->right));
            return rotateLeft(std::move(n));
        }
        return n;
    }
    unique_ptr<Node> insertNode(unique_ptr<Node> n, shared_ptr<Job> job) {
        if (!n) return make_unique<Node>(std::move(job));
        if (job->id < n->key) n->left = insertNode(std::move(n->left), std::move(job));
        else if (job->id > n->key) n->right = insertNode(std::move(n->right), std::move(job));
        else return n;
        return rebalance(std::move(n));
    }
    static Node* minimum(Node* n) {
        while (n && n->left) n = n->left.get();
        return n;
    }
    unique_ptr<Node> eraseNode(unique_ptr<Node> n, int key, bool& erased) {
        if (!n) return n;
        if (key < n->key) n->left = eraseNode(std::move(n->left), key, erased);
        else if (key > n->key) n->right = eraseNode(std::move(n->right), key, erased);
        else {
            erased = true;
            if (!n->left) return std::move(n->right);
            if (!n->right) return std::move(n->left);
            Node* successor = minimum(n->right.get());
            n->key = successor->key;
            n->job = successor->job;
            bool successorErased = false;
            n->right = eraseNode(std::move(n->right), successor->key, successorErased);
        }
        return rebalance(std::move(n));
    }
    static Job* findNode(Node* n, int key) {
        while (n) {
            if (key == n->key) return n->job.get();
            n = key < n->key ? n->left.get() : n->right.get();
        }
        return nullptr;
    }
    static void inorderNode(const Node* n, vector<int>& ids) {
        if (!n) return;
        inorderNode(n->left.get(), ids);
        ids.push_back(n->key);
        inorderNode(n->right.get(), ids);
    }
    static void inorderJobsNode(const Node* n, vector<shared_ptr<Job>>& jobs) {
        if (!n) return;
        inorderJobsNode(n->left.get(), jobs);
        jobs.push_back(n->job);
        inorderJobsNode(n->right.get(), jobs);
    }
public:
    Job* find(int id) const { return findNode(root.get(), id); }
    shared_ptr<Job> findShared(int id) const {
        const Node* n = root.get();
        while (n) {
            if (id == n->key) return n->job;
            n = id < n->key ? n->left.get() : n->right.get();
        }
        return nullptr;
    }
    bool contains(int id) const { return find(id) != nullptr; }
    void insert(const shared_ptr<Job>& job) {
        root = insertNode(std::move(root), job);
        ++insertions; ++recordCount;
    }
    bool erase(int id) {
        bool erased = false;
        root = eraseNode(std::move(root), id, erased);
        if (erased) { ++deletions; --recordCount; }
        return erased;
    }
    vector<shared_ptr<Job>> inorderJobs() const {
        vector<shared_ptr<Job>> result;
        inorderJobsNode(root.get(), result);
        return result;
    }
    void printStructure(ostream& out) const {
        out << "TREE\n";
        if (!root) { out << "END_TREE\n"; return; }
        vector<const Node*> level{root.get()};
        int levelNo = 0;
        while (true) {
            size_t lastReal = 0;
            bool hasReal = false;
            for (size_t i = 0; i < level.size(); ++i) {
                if (level[i]) { lastReal = i; hasReal = true; }
            }
            if (!hasReal) break;
            out << "LEVEL " << levelNo << ": ";
            vector<const Node*> next;
            for (size_t i = 0; i <= lastReal; ++i) {
                if (i) out << '|';
                const Node* n = level[i];
                if (n) {
                    out << '[' << n->key << ']';
                    next.push_back(n->left.get());
                    next.push_back(n->right.get());
                } else {
                    out << "NULL";
                    next.push_back(nullptr); next.push_back(nullptr);
                }
            }
            out << '\n';
            level = std::move(next);
            ++levelNo;
        }
        out << "END_TREE\n";
    }
    void printInorder(ostream& out) const {
        vector<int> ids; inorderNode(root.get(), ids);
        out << "INORDER: [";
        for (size_t i = 0; i < ids.size(); ++i) { if (i) out << ", "; out << ids[i]; }
        out << "]\n";
    }
    void printStats(ostream& out) const {
        out << "TREE_STATS\nrecords=" << recordCount << "\nheight=" << h(root)
            << "\ninsertions=" << insertions << "\ndeletions=" << deletions
            << "\nleft_rotations=" << leftRotations << "\nright_rotations=" << rightRotations
            << "\nEND_TREE_STATS\n";
    }
};

