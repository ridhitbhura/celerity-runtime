#pragma once

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "command.h"
#include "types.h"

namespace celerity {
namespace detail {

	class buffer_manager;
	class reduction_manager;
	class task_manager;

	// TODO: Could be extended (using SFINAE) to support additional iterator types (e.g. random access)
	template <typename Iterator, typename PredicateFn>
	class filter_iterator {
	  public:
		using value_type = typename std::iterator_traits<Iterator>::value_type;
		using difference_type = typename std::iterator_traits<Iterator>::difference_type;
		using reference = typename std::iterator_traits<Iterator>::reference;
		using pointer = typename std::iterator_traits<Iterator>::pointer;
		using iterator_category = std::forward_iterator_tag;

		filter_iterator(Iterator begin, Iterator end, PredicateFn fn) : m_it(begin), m_end(end), m_fn(fn) { advance(); }

		bool operator!=(const filter_iterator& rhs) { return m_it != rhs.m_it; }

		reference operator*() { return *m_it; }
		reference operator->() { return *m_it; }

		filter_iterator& operator++() {
			if(m_it != m_end) {
				++m_it;
				advance();
			}
			return *this;
		}

	  private:
		Iterator m_it;
		const Iterator m_end;
		PredicateFn m_fn;

		void advance() {
			while(m_it != m_end && !m_fn(*m_it)) {
				++m_it;
			}
		}
	};

	template <typename Iterator, typename PredicateFn>
	filter_iterator<Iterator, PredicateFn> make_filter_iterator(Iterator begin, Iterator end, PredicateFn fn) {
		return filter_iterator<Iterator, PredicateFn>(begin, end, fn);
	}

	// TODO: Could be extended (using SFINAE) to support additional iterator types (e.g. random access)
	template <typename Iterator, typename TransformFn>
	class transform_iterator {
	  public:
		using value_type = decltype(std::declval<TransformFn>()(std::declval<typename std::iterator_traits<Iterator>::reference>()));
		using difference_type = typename std::iterator_traits<Iterator>::difference_type;
		using reference = value_type; // We cannot return a reference (but this is OK according to the standard)
		using pointer = value_type*;
		using iterator_category = std::forward_iterator_tag;

		transform_iterator(Iterator it, TransformFn fn) : m_it(it), m_fn(fn) {}

		bool operator!=(const transform_iterator& rhs) { return m_it != rhs.m_it; }

		reference operator*() { return m_fn(*m_it); }
		reference operator->() { return m_fn(*m_it); }

		transform_iterator& operator++() {
			++m_it;
			return *this;
		}

	  private:
		Iterator m_it;
		TransformFn m_fn;
	};

	template <typename Iterator, typename TransformFn>
	transform_iterator<Iterator, TransformFn> make_transform_iterator(Iterator it, TransformFn fn) {
		return transform_iterator<Iterator, TransformFn>(it, fn);
	}

	class command_graph {
	  public:
		template <typename T, typename... Args>
		T* create(Args&&... args) {
			static_assert(std::is_base_of<abstract_command, T>::value, "T must be derived from abstract_command");
			auto unique_cmd = std::unique_ptr<T>{new T(m_next_cmd_id++, std::forward<Args>(args)...)}; // new, because ctors are private, but we are friends
			const auto cmd = unique_cmd.get();
			m_commands.emplace(std::pair{cmd->get_cid(), std::move(unique_cmd)});
			if constexpr(std::is_base_of_v<task_command, T>) { m_by_task[cmd->get_tid()].emplace_back(cmd); }
			m_execution_front.insert(cmd);
			return cmd;
		}

		void erase(abstract_command* cmd);

		void erase_if(std::function<bool(abstract_command*)> condition);

		bool has(command_id cid) const { return m_commands.count(cid) == 1; }

		abstract_command* get(command_id cid) { return m_commands.at(cid).get(); }

		template <typename T>
		T* get(command_id cid) {
			// dynamic_cast with reference to force bad_cast to be thrown if type mismatches
			return &dynamic_cast<T&>(*m_commands.at(cid));
		}

		size_t command_count() const { return m_commands.size(); }
		size_t task_command_count(task_id tid) const {
			if(m_by_task.count(tid) == 0) return 0;
			return m_by_task.at(tid).size();
		}

		auto all_commands() const {
			const auto transform = [](auto& uptr) { return uptr.second.get(); };
			return iterable_range{make_transform_iterator(m_commands.cbegin(), transform), make_transform_iterator(m_commands.cend(), transform)};
		}

		auto& task_commands(task_id tid) { return m_by_task.at(tid); }

		void add_dependency(abstract_command* depender, abstract_command* dependee, dependency_kind kind, dependency_origin origin) {
			assert(dependee != depender);
			depender->add_dependency({dependee, kind, origin});
			m_execution_front.erase(dependee);

			// Sanity check: For non-dataflow dependencies the commands can only be of specific types
			if(origin == dependency_origin::execution_front) { assert(utils::isa<epoch_command>(depender) || utils::isa<horizon_command>(depender)); }
			if(origin == dependency_origin::collective_group_serialization) {
				assert(utils::isa<execution_command>(depender));
				// The original execution command may have been subsumed by a horizon / epoch
				assert(utils::isa<execution_command>(dependee) || utils::isa<epoch_command>(dependee) || utils::isa<horizon_command>(dependee));
			}
			if(origin == dependency_origin::last_epoch) { assert(utils::isa<epoch_command>(dependee) || utils::isa<horizon_command>(dependee)); }

			// Sanity check for unit tests, where we may have multiple CDAGS
			assert(m_commands.at(depender->get_cid()).get() == depender);
			assert(m_commands.at(dependee->get_cid()).get() == dependee);
		}

		void remove_dependency(abstract_command* depender, abstract_command* dependee) { depender->remove_dependency(dependee); }

		const std::unordered_set<abstract_command*>& get_execution_front() const { return m_execution_front; }

	  private:
		command_id m_next_cmd_id = 0;
		// TODO: Consider storing commands in a contiguous memory data structure instead
		std::unordered_map<command_id, std::unique_ptr<abstract_command>> m_commands;
		std::unordered_map<task_id, std::vector<task_command*>> m_by_task;

		std::unordered_set<abstract_command*> m_execution_front;
	};

} // namespace detail
} // namespace celerity
