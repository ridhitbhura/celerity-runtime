#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>

#include <spdlog/fmt/ostr.h> // Enable formatting of types that support operator<<(std::ostream&, T)
#include <spdlog/spdlog.h>

#include "print_utils.h"

#define CELERITY_LOG_SET_SCOPED_CTX(ctx) CELERITY_DETAIL_LOG_SET_SCOPED_CTX(ctx)

#define CELERITY_LOG(level, ...)                                                                                                                               \
	(::spdlog::should_log(level)                                                                                                                               \
	        ? SPDLOG_LOGGER_CALL(::spdlog::default_logger_raw(), level, "{}{}", *::celerity::detail::active_log_ctx, ::fmt::format(__VA_ARGS__))               \
	        : (void)0)

// TODO Add a macro similar to SPDLOG_ACTIVE_LEVEL, configurable through CMake
#define CELERITY_TRACE(...) CELERITY_LOG(::celerity::detail::log_level::trace, __VA_ARGS__)
#define CELERITY_DEBUG(...) CELERITY_LOG(::celerity::detail::log_level::debug, __VA_ARGS__)
#define CELERITY_INFO(...) CELERITY_LOG(::celerity::detail::log_level::info, __VA_ARGS__)
#define CELERITY_WARN(...) CELERITY_LOG(::celerity::detail::log_level::warn, __VA_ARGS__)
#define CELERITY_ERROR(...) CELERITY_LOG(::celerity::detail::log_level::err, __VA_ARGS__)
#define CELERITY_CRITICAL(...) CELERITY_LOG(::celerity::detail::log_level::critical, __VA_ARGS__)

namespace celerity {
namespace detail {

	using log_level = spdlog::level::level_enum;

	template <typename... Es>
	struct log_map {
		const std::tuple<Es...>& entries;
		log_map(const std::tuple<Es...>& entries) : entries(entries) {}
	};

	struct log_context {
		std::string value;
		log_context() = default;
		template <typename... Es>
		explicit log_context(const std::tuple<Es...>& entries) {
			static_assert(sizeof...(Es) % 2 == 0, "log_context requires key/value pairs");
			value = fmt::format("[{}] ", log_map{entries});
		}
	};

	inline const std::string null_log_ctx;
	inline thread_local const std::string* active_log_ctx = &null_log_ctx;

	struct log_ctx_setter {
		log_ctx_setter(log_context& ctx) { celerity::detail::active_log_ctx = &ctx.value; }
		~log_ctx_setter() { celerity::detail::active_log_ctx = &celerity::detail::null_log_ctx; }
	};

#define CELERITY_DETAIL_LOG_SET_SCOPED_CTX(ctx)                                                                                                                \
	log_ctx_setter _set_log_ctx_##__COUNTER__ { ctx }

	template <typename Tuple, typename Callback>
	constexpr void tuple_for_each_pair_impl(const Tuple&, Callback&&, std::index_sequence<>) {}

	template <typename Tuple, size_t I1, size_t I2, size_t... Is, typename Callback>
	constexpr void tuple_for_each_pair_impl(const Tuple& tuple, const Callback& cb, std::index_sequence<I1, I2, Is...>) {
		cb(std::get<I1>(tuple), std::get<I2>(tuple));
		tuple_for_each_pair_impl(tuple, cb, std::index_sequence<Is...>{});
	}

	template <typename Tuple, typename Callback>
	constexpr void tuple_for_each_pair(const Tuple& tuple, const Callback& cb) {
		static_assert(std::tuple_size_v<Tuple> % 2 == 0, "an even number of entries is required");
		tuple_for_each_pair_impl(tuple, cb, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
	}

} // namespace detail
} // namespace celerity

template <typename... Es>
struct fmt::formatter<celerity::detail::log_map<Es...>> {
	constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const celerity::detail::log_map<Es...>& map, FormatContext& ctx) {
		auto&& out = ctx.out();
		int i = 0;
		tuple_for_each_pair(map.entries, [&i, &out](auto& a, auto& b) {
			if(i++ > 0) { fmt::format_to(out, ", "); }
			fmt::format_to(out, "{}={}", a, b);
		});
		return out;
	}
};
