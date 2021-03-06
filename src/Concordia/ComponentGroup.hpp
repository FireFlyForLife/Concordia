#pragma once
#include "Identification.hpp"
#include <array>
#include <iostream>
#include <string>
#include "MetaDebugging.hpp"
#include <type_traits>

namespace Concordia
{
	namespace Impl
	{
		template<typename T>
		constexpr size_t get_id_from()
		{
			using TClean = typename std::decay<T>::type;
			return get_cmp_id<TClean>();
		}

		template<typename T, typename = typename std::enable_if<!std::is_pointer<T>::value>::type>
		constexpr void* to_void_ptr(T& t)
		{
			return static_cast<void*>(&t);
		}

		template<typename T, typename = std::enable_if_t<std::is_pointer<T>::value>>
		constexpr void* to_void_ptr(T t)
		{
			return static_cast<void*>(t);
		}

		template<typename T, int N, typename... Ts>
		struct get_index_in_pack_impl;

		template<typename T, int N, typename T0, typename... Ts>
		struct get_index_in_pack_impl<T, N, T0, Ts...>
		{
			using TClean = std::decay_t<T>;
			using T0Clean = std::decay_t<T0>;
			static constexpr int value = std::is_same<TClean, T0Clean>::value ? N : get_index_in_pack_impl<T, (N + 1), Ts...>::value;
		};

		template<typename T, int N, typename T0>
		struct get_index_in_pack_impl<T, N, T0>
		{
			using TClean = std::decay_t<T>;
			using T0Clean = std::decay_t<T0>;
			static constexpr int value = std::is_same<TClean, T0Clean>::value ? N : -1;
		};

		template<typename T, int N>
		struct get_index_in_pack_impl<T, N>
		{
			static constexpr int value = -1;
		};

		template<typename T, typename... Ts>
		constexpr int get_index_in_pack = get_index_in_pack_impl<T, 0, Ts...>::value;

		template<size_t I, typename... Pack>
		struct n_type_in_pack_impl;

		template<size_t I, typename T, typename... Pack>
		struct n_type_in_pack_impl<I, T, Pack...>
		{
			using type = typename n_type_in_pack_impl<I - 1, Pack...>::type;
		};

		template<typename T, typename... Pack>
		struct n_type_in_pack_impl<0, T, Pack...>
		{
			using type = T;
		};

		template<typename T>
		struct n_type_in_pack_impl<0, T>
		{
			using type = T;
		};

		//TODO: Add static_assert range check
		//TODO: Use the one in MetaUtils.hpp
		template<size_t I, typename... Pack>
		using nth_type_in_pack = typename n_type_in_pack_impl<I, Pack...>::type;
	}

	/// 	Immutable group of components
	/// This is used for when you want to have a bunch of components and want
	/// it to only be 1 parameter
	template<typename ...Args>
	struct ComponentGroup
	{
		static constexpr size_t size = sizeof...(Args);

		static const std::array<size_t, size> ids;

		std::array<void*, size> cmps;

		constexpr ComponentGroup(Args&... args)
			: cmps{ Impl::to_void_ptr(args)... }
		{
		}

		template<typename T>
		T& get()
		{
			constexpr int index = Impl::get_index_in_pack<T, Args...>;

			static_assert(index != -1, "T was not found in GetNInList, function signature: " FUNCTION_STR);
			if (index == -1)
				assert(false && "This should never happen because of the static_assert");

			return *static_cast<T*>(cmps[index]);
		}

		template<typename T>
		T& get() const
		{
			constexpr int index = Impl::get_index_in_pack<T, Args...>;

			static_assert(index != -1, "GetNInTypelist is trying to access a type which is not available");
			if (index == -1)
				assert(false && "generate an error if the static assert did not do it already");

			void* cmp = cmps[index];
			return *static_cast<T*>(cmp);
		}

		/// Gets the n-th component from this group.
		/// Required for structured binding support in c++17
		template<std::size_t I>
		auto& get()
		{
			static_assert(I < size, "I is bigger than the amount of components");

			using CmpType = Impl::nth_type_in_pack<I, Args...>;

			return *static_cast<CmpType*>(cmps[I]);
		}

		/// Gets the n-th const component from this group.
		/// Required for structured binding support in c++17
		template<std::size_t I>
		auto& get() const
		{
			static_assert(I < size, "I is bigger than the amount of components");

			using CmpType = Impl::nth_type_in_pack<I, Args...>;

			return *static_cast<CmpType*>(cmps[I]);
		}

		void extractInto(Args*... Cs)
		{
			extract_impl(std::forward<Args*>(Cs)...);
		}

	private:
		template<typename C, typename... Cs>
		void extract_impl(C* c, Cs*... cs)
		{
			*c = get<C>();
			extract_impl(std::forward<Cs*>(cs)...);
		}

		template<typename C>
		void extract_impl(C* c)
		{
			*c = get<C>();
		}
	};

	template<typename... Cmps>
	constexpr std::array<size_t, ComponentGroup<Cmps...>::size> pack_get_ids()
	{
		return { Impl::get_id_from<Cmps>()... };
	}

	template<typename... Cmps>
	const std::array<size_t, ComponentGroup<Cmps...>::size> ComponentGroup<Cmps...>::ids = pack_get_ids<Cmps...>();
}


namespace std
{
	/// We specialize these functions so structured bindings work in c++17 for our ComponentGroup

	template<typename... Types>
	struct tuple_size<Concordia::ComponentGroup<Types...>> : public integral_constant<size_t, sizeof...(Types)> {};

	template<std::size_t N, typename... Types>
	struct tuple_element<N, Concordia::ComponentGroup<Types...>> {
		//using type = decltype( std::declval<Concordia::ComponentGroup<Types...>>().template get<N>());
		using type = Concordia::Impl::nth_type_in_pack<N, Types...>;
	};
}
