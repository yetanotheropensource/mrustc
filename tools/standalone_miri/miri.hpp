/*
 * mrustc Standalone MIRI
 * - by John Hodge (Mutabah)
 *
 * miri.hpp
 * - MIR Interpreter State (HEADER)
 */
#pragma once
#include "module_tree.hpp"
#include "value.hpp"

struct ThreadState
{
    static unsigned s_next_tls_key;
    unsigned call_stack_depth;
    ::std::vector< ::std::pair<uint64_t, RelocationPtr> > tls_values;

    unsigned    panic_count;
    bool    panic_active;
    Value   panic_value;

    ThreadState():
        call_stack_depth(0)
        ,panic_count(0)
        ,panic_active(false)
    {
    }

    struct DecOnDrop {
        unsigned* p;
        ~DecOnDrop() { (*p) --; }
    };
    DecOnDrop enter_function() {
        this->call_stack_depth ++;
        return DecOnDrop { &this->call_stack_depth };
    }
};

class InterpreterThread
{
    friend struct MirHelpers;
    struct StackFrame
    {
        static unsigned s_next_frame_index;
        unsigned    frame_index;

        ::std::function<bool(Value&,Value)> cb;
        const Function* fcn;
        Value ret;
        ::std::vector<Value>    args;
        ::std::vector<Value>    locals;
        ::std::vector<bool>     drop_flags;

        unsigned    bb_idx;
        unsigned    stmt_idx;

        StackFrame(const Function& fcn, ::std::vector<Value> args);
        static StackFrame make_wrapper(::std::function<bool(Value&,Value)> cb) {
            static Function f;
            StackFrame  rv(f, {});
            rv.cb = ::std::move(cb);
            return rv;
        }
    };

    ModuleTree& m_modtree;
    ThreadState m_thread;
    size_t  m_instruction_count;
    ::std::vector<StackFrame>   m_stack;

public:
    InterpreterThread(ModuleTree& modtree):
        m_modtree(modtree),
        m_instruction_count(0)
    {
    }
    ~InterpreterThread();

    void start(const RcString& p, ::std::vector<Value> args);
    // Returns `true` if the call stack empties
    bool step_one(Value& out_thread_result);

private:
    bool pop_stack(Value& out_thread_result);

    // Returns true if the call was resolved instantly
    bool call_path(Value& ret_val, const HIR::Path& p, ::std::vector<Value> args);
    // Returns true if the call was resolved instantly
    bool call_extern(Value& ret_val, const ::std::string& name, const ::std::string& abi, ::std::vector<Value> args);
    // Returns true if the call was resolved instantly
    bool call_intrinsic(Value& ret_val, const ::HIR::TypeRef& ret_ty, const RcString& name, const ::HIR::PathParams& pp, ::std::vector<Value> args);

    // Returns true if the call was resolved instantly
    bool drop_value(Value ptr, const ::HIR::TypeRef& ty, bool is_shallow=false);
};

