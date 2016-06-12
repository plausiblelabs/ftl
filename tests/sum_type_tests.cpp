/*
 * Copyright (c) 2013 Björn Aili
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 */
#include <type_traits>
#include <memory>
#include <iostream>
#include <ftl/sum_type.h>
#include "sum_type_tests.h"

template<typename T>
struct Just {
	explicit constexpr Just(const T& t) noexcept : value(t) {}
	explicit constexpr Just(T&& t) noexcept : value(std::move(t)) {}

	T& operator*() {
		return value;
	}

	constexpr const T& operator*() const {
		return value;
	}

private:
	T value;
};

struct Nothing {};

template<typename T>
using maybe = ftl::sum_type<Just<T>,Nothing>;

template<typename T>
maybe<ftl::plain_type<T>> just(T&& t) {
	using ftl::plain_type;

	return maybe<plain_type<T>>{
		ftl::type<Just<plain_type<T>>>, std::forward<T>(t)
	};
}

struct NonTrivial
{
	constexpr NonTrivial() noexcept : field(0) {}
	NonTrivial(const NonTrivial&) = default;
	NonTrivial(NonTrivial&&) = default;
	explicit constexpr NonTrivial(int x) : field(x) {}
	~NonTrivial() {}

	NonTrivial& operator= (const NonTrivial& other) = default;
	NonTrivial& operator= (NonTrivial&& other) = default;

	int field;
};

bool operator== (NonTrivial lhs, NonTrivial rhs) noexcept
{
	return lhs.field == rhs.field;
}

struct CopyThrow
{
	CopyThrow() = default;
	CopyThrow(const CopyThrow&)
	{
		throw std::exception();
	}
	CopyThrow(CopyThrow&&) = default;
	~CopyThrow() = default;
};

constexpr bool wasInt(int x)
{
	return x == 5;
}

constexpr bool wasNotChar(char)
{
	return false;
}

constexpr bool wasNotNothing(Nothing)
{
	return false;
}

test_set sum_type_tests{
	std::string("sum_type"),
	{
		std::make_tuple(
			std::string("Static assertions"),
			std::function<bool()>([]() -> bool {
				using namespace ftl;

				static_assert(std::is_literal_type<sum_type<int,char>>::value, "Sum type of trivial types should be guaranteed literal");
				static_assert(std::is_trivial<sum_type<int,char>>::value, "Sum type of trivial types should be guaranteed trivial");

				static_assert(!std::is_literal_type<sum_type<NonTrivial,char>>::value, "Sum types containing non-trivial types should not be literal");
				static_assert(!std::is_trivial<sum_type<NonTrivial,char>>::value, "Sum types containing non-trivial types should not be trivial");

				constexpr sum_type<int,char> x{type<int>, 5};

				static_assert(
					x.match(wasInt, wasNotChar),
					"Match expressions with constexpr case clauses should be constexpr"
 				);

				constexpr auto y = x;

				static_assert(
					y.match(wasInt, wasNotChar),
					"Match expressions with constexpr case clauses should be constexpr"
				);

				constexpr sum_type<Nothing,int> z{type<int>, 5};

				static_assert(
					z.match(wasNotNothing, wasInt),
					"Match expressions with constexpr case clauses should be constexpr"
				);

				return true;
			})
		),
		std::make_tuple(
			std::string("Construct using constructor tag"),
			std::function<bool()>([]() -> bool {
				using namespace ftl;

				int x = 1;

				// Trivial sum type
				sum_type<int,int*> y{type<int>, 12};
				sum_type<int,int*> z{type<int*>, &x};

				// Non-trivial sum type
				sum_type<int,NonTrivial> a{type<int>, 12};
				sum_type<int,NonTrivial> b{type<NonTrivial>, 12};

				return true;
			})
		),
		std::make_tuple(
			std::string("is<T>"),
			std::function<bool()>([]() -> bool {
				using namespace ftl;

				sum_type<int,char> x{type<int>, 10};
				sum_type<int,char> y{type<char>, 'b'};

				return x.is<int>() && !x.is<char>()
					&& !y.is<int>() && y.is<char>();
			})
		),
		std::make_tuple(
			std::string("Eq"),
			std::function<bool()>([]() -> bool {
				using namespace ftl;

				sum_type<int,char> w{type<int>, 12};
				sum_type<int,char> x{type<int>, 10};
				sum_type<int,char> y{type<char>, 'b'};
				sum_type<int,char> z{type<int>, 10};

				return w != x && x != y && x == z && w != y;
			})
		),
		std::make_tuple(
			std::string("Copy assign"),
			std::function<bool()>([]() -> bool {
				using namespace ftl;

				// Copy assign trivial sum types
				sum_type<int,char> x1{type<int>, 1};
				sum_type<int,char> y1{type<int>, 5};
				sum_type<int,char> z1{type<char>, 'a'};

				x1 = y1;
				y1 = z1;

				// Copy assign non-trivial sum types
				sum_type<std::shared_ptr<int>,NonTrivial> x2{type<NonTrivial>, 1};
				sum_type<std::shared_ptr<int>,NonTrivial> y2{type<NonTrivial>, 10};
				sum_type<std::shared_ptr<int>,NonTrivial> z2{type<std::shared_ptr<int>>, new int(15)};

				x2 = y2;
				y2 = z2;

				return x1.unsafe_get<int>() == 5
					&& y1.unsafe_get<char>() == 'a'
					&& x2.unsafe_get<NonTrivial>() == NonTrivial(10)
					&& *y2.unsafe_get<std::shared_ptr<int>>() == 15;
			})
		),
		std::make_tuple(
			std::string("Copy assign element types"),
			std::function<bool()>([]() -> bool {
				using namespace ftl;

				// Copy assign trivial sum types
				sum_type<int,char> x1{type<int>, 1};
				sum_type<int,char> y1{type<int>, 5};

				x1 = 5;
				y1 = 'a';

				// Copy assign non-trivial sum types
				sum_type<std::shared_ptr<int>,NonTrivial> x2{type<NonTrivial>, 1};
				sum_type<std::shared_ptr<int>,NonTrivial> y2{type<NonTrivial>, 10};

				x2 = NonTrivial{10};
				y2 = std::shared_ptr<int>(new int(15));

				return x1.unsafe_get<int>() == 5
					&& y1.unsafe_get<char>() == 'a'
					&& x2.unsafe_get<NonTrivial>() == NonTrivial(10)
					&& *y2.unsafe_get<std::shared_ptr<int>>() == 15;
			})
		),
		std::make_tuple(
			std::string("Copy assign with throwing constructor"),
			std::function<bool()>([]() -> bool {
				using namespace ftl;

				auto x = sum_type<CopyThrow, int>{type<CopyThrow>};
				auto y = sum_type<CopyThrow, int>{type<int>, 5};

				x =
					y.match(
						[](int val) { return val; },
						[](otherwise) { return 0; }
					);

				return x.unsafe_get<int>() == 5;
			})
		),
		std::make_tuple(
			std::string("Move assign"),
			std::function<bool()>([]() -> bool {
				using namespace ftl;

				// Move assign trivial sum types
				sum_type<int,char> w1{type<int>, 1};
				sum_type<int,char> x1{type<int>, 1};
				sum_type<int,char> y1{type<int>, 5};
				sum_type<int,char> z1{type<char>, 'a'};

				x1 = std::move(y1);
				w1 = std::move(z1);

				// Move assign non-trivial sum types
				sum_type<int,NonTrivial> w2{type<NonTrivial>, 1};
				sum_type<int,NonTrivial> x2{type<NonTrivial>, 1};
				sum_type<int,NonTrivial> y2{type<NonTrivial>, 10};
				sum_type<int,NonTrivial> z2{type<int>, 15};

				x2 = std::move(y2);
				w2 = std::move(z2);

				return x1.unsafe_get<int>() == 5
					&& w1.unsafe_get<char>() == 'a'
					&& x2.unsafe_get<NonTrivial>() == NonTrivial(10)
					&& w2.unsafe_get<int>() == 15;
			})
		),
		std::make_tuple(
			std::string("Get by type"),
			std::function<bool()>([]() -> bool {
				using namespace ftl;

				sum_type<int,char> x{type<int>, 10};
				sum_type<int,char> y{type<char>, 'b'};

				auto s1 = x.unsafe_get<int>();
				auto s2 = y.unsafe_get<char>();

				return s1 == 10 && s2 == 'b';
			})
		),
		std::make_tuple(
			std::string("Match expressions (trivial types)"),
			std::function<bool()>([]() -> bool {
				using namespace ftl;

				struct A {};
				struct B {};
				struct C {};

				sum_type<A,B,C> x{type<A>};
				sum_type<A,B,C> y{type<B>};
				sum_type<A,B,C> z{type<C>};

				auto s1 = x.match(
					[](A){ return 0; },
					[](B){ return 1; },
					[](C){ return 2; }
				);

				auto s2 = y.match(
					[](const A&){ return 0; },
					[](const B&){ return 1; },
					[](const C&){ return 2; }
				);

				auto s3 = z.match(
					[](A){ return 0; },
					[](B){ return 1; },
					[](C){ return 2; }
				);

				return s1 == 0 && s2 == 1 && s3 == 2;
			})
		),
		std::make_tuple(
			std::string("Match expressions (non-trivial types)"),
			std::function<bool()>([]() -> bool {
				using namespace ftl;

				struct A {};
				struct B {};

				sum_type<NonTrivial,A,B> x{type<NonTrivial>};
				sum_type<A,NonTrivial,B> y{type<A>};
				sum_type<A,B,NonTrivial> z{type<B>};

				auto s1 = x.match(
					[](A){ return 0; },
					[](B){ return 1; },
					[](NonTrivial){ return 2; }
				);

				auto s2 = y.match(
					[](const A&){ return 0; },
					[](const B&){ return 1; },
					[](const NonTrivial&){ return 2; }
				);

				auto s3 = z.match(
					[](A){ return 0; },
					[](B){ return 1; },
					[](NonTrivial){ return 2; }
				);

				return s1 == 2 && s2 == 0 && s3 == 1;
			})
		),
		std::make_tuple(
			std::string("Match with otherwise (trivial types)"),
			std::function<bool()>([]() -> bool {
				using namespace ftl;

				struct A {};
				struct B {};
				struct C {};

				sum_type<A,B,C> x{type<A>};
				sum_type<A,B,C> y{type<B>};
				sum_type<A,B,C> z{type<C>};

				auto s1 = x.match(
					[](A){ return 0; },
					[](otherwise){ return 1; }
				);

				auto s2 = y.match(
					[](const A&){ return 0; },
					[](otherwise){ return 1; }
				);

				auto s3 = z.match(
					[](A){ return 0; },
					[](otherwise){ return 1; }
				);

				return s1 == 0 && s2 == 1 && s3 == 1;
			})
		),
		std::make_tuple(
			std::string("Match expressions [&]"),
			std::function<bool()>([]() -> bool {
				using namespace ftl;

				struct A {};

				sum_type<A,int> x{type<int>, 5};

				auto r = x.match(
					[](A&){ return 0; },
					[](int& i){ ++i; return i; }
				);

				return r == x.unsafe_get<int>();
			})
		),
		std::make_tuple(
			std::string("Match expressions [void]"),
			std::function<bool()>([]() -> bool {
				using namespace ftl;

				struct A {};

				int i1 = 5, i2 = 10;

				sum_type<A,int> x{type<int>, 5};
				const sum_type<A,int> y{type<A>};

				x.match(
					[&](A&){ ++i1; },
					[&](int&){ ++i2; }
				);

				y.match(
					[&](const A&){ ++i1; },
					[&](const int&){ ++i2; }
				);

				return i1 == 6 && i2 == 11;
			})
		),
		std::make_tuple(
			std::string("Maybe mockup"),
			std::function<bool()>([]() -> bool {
				using namespace ftl;

				auto x = just(12);
				auto y = maybe<int>{type<Nothing>};

				auto s1 = x.match(
					[](Just<int> x){ return *x; },
					[](Nothing){ return 0; }
				);

				return s1 == 12;
			})
		),
	}
};

