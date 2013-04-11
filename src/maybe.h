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
#ifndef FTL_MAYBE_H
#define FTL_MAYBE_H

#include <stdexcept>
#include "type_functions.h"

namespace ftl {

	/**
	 * Abstracts the concept of optional arguments and similar.
	 *
	 * In essence, an instance of maybe is either a value, or nothing.
	 * 
	 * \par Concepts
	 * Maybe is an instance of the following concepts:
	 * \li DefaultConstructible
	 * \li CopyConstructible
	 * \li MoveConstructible
	 * \li Assignable
	 * \li Dereferencable
	 * \li EqComparable, iff A is EqComparable
	 * \li Orderable, iff A is Orderable
	 * \li Functor (in A)
	 * \li Monad (in A)
	 * \li Monoid, iff A is Monoid
	 */
	template<typename A>
	class maybe {
	public:
		/**
		 * Compatibility typedef.
		 *
		 * Allows compatibility with plethora of templated functions/structures
		 * that require an object have a value_type member.
		 */
		using value_type = A;

		/**
		 * Default c-tor, equivalent to \c nothing.
		 *
		 * Memory for the contained type is reserved on the stack, but no
		 * initialisation is done. In other words, A's constructor is \em not
		 * called.
		 */
		constexpr maybe() noexcept {}

		maybe(const maybe& m)
		noexcept(std::is_nothrow_copy_constructible<A>::value)
	    : isValid(m.isValid) {
			if(isValid) {
				new (&val) value_type(m.val);
			}
		}

		maybe(maybe&& m)
		noexcept(std::is_nothrow_move_constructible<A>::value)
		: isValid(m.isValid) {
			if(isValid) {
				new (&val) value_type(std::move(m.val));
				m.isValid = false;
			}
		}

		explicit constexpr maybe(const value_type& v)
		noexcept(std::is_nothrow_copy_constructible<A>::value)
		: isValid(true), val(v) {}

		explicit constexpr maybe(const value_type&& v)
		noexcept(std::is_nothrow_move_constructible<A>::value)
		: isValid(true), val(std::move(v)) {}

		// TODO: Enable the noexcept specifier once is_nothrow_destructible is
		// available.
		~maybe() /*noexcept(std::is_nothrow_destructible<A>::value)*/ {
			if(isValid) {
				val.~value_type();
				isValid = false;
			}
		}

		/**
		 * Check if the maybe is nothing.
		 */
		constexpr bool isNothing() noexcept {
			return !isValid;
		}

		/**
		 * Check if the maybe is a value.
		 */
		constexpr bool isValue() noexcept {
			return isValid;
		}

		const maybe& operator= (const maybe& m)
		/* TODO: Enable noexcept specifier once is_nothrow_destructible is
		 * available.
		noexcept(  std::is_nothrow_copy_constructible<A>::value
				&& std::is_nothrow_destructible<A>::value) */ {
			// Check for self-assignment
			if(this == &m)
				return;

			if(isValid) {
				val.~value_type();
			}

			isValid = m.isValid;
			if(isValid) {
				new (&val) value_type(m.val);
			}

			return *this;
		}

		const maybe& operator= (maybe&& m)
		/* TODO: Enable noexcept specifier once is_nothrow_destructible is
		 * available.
		noexcept(  std::is_nothrow_copy_constructible<A>::value
				&& std::is_nothrow_destructible<A>::value) */ {
			// Check for self-assignment
			if(this == &m)
				return;

			if(isValid) {
				val.~value_type();
			}

			isValid = m.isValid;
			if(isValid) {
				new (&val) value_type(std::move(m.val));
				m.isValid = false;
			}

			return *this;
		}

		/**
		 * Bool conversion operator.
		 *
		 * Provided for convenience, to allow syntax such as
		 * \code
		 *   maybe<T> m = ...;
		 *   if(m) {
		 *       doStuff(m);
		 *   }
		 * \endcode
		 */
		constexpr operator bool() noexcept {
			return isValue();
		}

		/**
		 * Dereference operator.
		 * 
		 * \note Throws an \c std::logic_error if \c this is \c nothing.
		 */
		value_type& operator* () {
			if(!isValid)
				throw std::logic_error("Attempting to read the value of Nothing.");

			return val;
		}

		/**
		 * \overload
		 */
		const value_type& operator* () const {
			if(!isValid)
				throw std::logic_error("Attempting to read the value of Nothing.");

			return val;
		}

		/**
		 * Member access operator.
		 * 
		 * \note Throws an \c std::logic_error if \c this is \c nothing.
		 */
		value_type* operator-> () {
			if(!isValid)
				throw std::logic_error("Attempting to read teh value of Nothing.");

			return &val;
		}

		/// \overload
		const value_type* operator-> () const {
			if(!isValid)
				throw std::logic_error("Attempting to read teh value of Nothing.");

			return &val;
		}

		/**
		 * Static constructor of Nothing:s.
		 */
		static constexpr maybe<A> nothing() noexcept {
			return maybe();
		}

	private:
		// Dummy union to prevent value from being initialised except when needed
		union {
			char dummy[sizeof(value_type)];
			value_type val;
		};

		bool isValid = false;
	};

	/**
	 * Convenience function to create maybe:s.
	 */
	template<typename A>
	constexpr maybe<A> value(const A& a)
	noexcept(std::is_nothrow_copy_constructible<A>::value) {
		return maybe<A>(a);
	}

	/**
	 * \overload
	 */
	template<typename A>
	constexpr maybe<A> value(A&& a)
	noexcept(std::is_nothrow_move_constructible<A>::value) {
		return maybe<A>(std::move(a));
	}

	/**
	 * EqComparable::operator== implementation for maybe.
	 *
	 * Will result in compilation error if A is not EqComparable.
	 */
	template<typename A>
	bool operator== (const maybe<A>& m1, const maybe<A>& m2) {
		if(m1 && m2) {
			return *m1 == *m2;
		}
		else if(!m1 && !m2) {
			return true;
		}
		else {
			return false;
		}
	}

	/**
	 * EqComparable::operator!= implementation for maybe.
	 *
	 * Will result in compilation error if A is not EqComparable.
	 */
	template<typename A>
	bool operator!= (const maybe<A>& m1, const maybe<A>& m2) {
		return !(m1 == m2);
	}

	/**
	 * Orderable::operator< implementation for maybe.
	 *
	 * Will result in compilation error if A is not also Orderable.
	 */
	template<typename A>
	bool operator< (const maybe<A>& m1, const maybe<A>& m2) {
		if(m1) {
			if(m2) {
				return *m1 < *m2;
			}
			else {
				return false;
			}
		}
		else if(m2) {
			return true;
		}
		else {
			return false;
		}
	}

	/**
	 * Orderable::operator> implementation for maybe.
	 */
	template<typename A>
	bool operator> (const maybe<A>& m1, const maybe<A>& m2) {
		if(m1) {
			if(m2) {
				return *m1 > *m2;
			}
			else {
				return true;
			}
		}
		else if(m2) {
			return false;
		}
		else {
			return false;
		}
	}

	/**
	 * Functor instance for maybe.
	 *
	 * Maps a function to the contained value, if it is a value. If it's
	 * Nothing, then Nothing is returned.
	 */
	template<
		typename F,
		typename A,
		typename B = typename decayed_result<F(A)>::type>
	maybe<B> fmap(const F& f, const maybe<A>& m) {
		return m ? maybe<B>(f(*m)) : maybe<B>::nothing();
	}

	template<
		typename A,
		typename R,
		typename...Ps>
	maybe<A>& fmap(R (A::*method)(Ps...), maybe<A>& m, Ps&&...ps) {
		if(m) {
			((*m).*method)(std::forward(ps)...);
		}

		return m;
	}

	/**
	 * Monoid implementation for maybe.
	 *
	 * Semantics are:
	 * \code
	 *   id() <=> maybe::nothing() <=> maybe()
	 *   append(value(x), value(y)) <=> value(append(x, y))
	 *   append(value(x), maybe::nothing()) <=> value(x)
	 *   append(maybe::nothing, value(y)) <=> value(y)
	 *   append(maybe::nothing, maybe::nothing) <=> maybe::nothing
	 * \endcode
	 *
	 * In other words, the append operation is simply lifted into the
	 * \c value_type of the maybe and all nothings are ignored (unless
	 * everything is nothing).
	 */
	template<typename A>
	struct monoid<maybe<A>> {
		static constexpr maybe<A> id() {
			return maybe<A>();
		}

		static maybe<A> append(const maybe<A>& m1, const maybe<A>& m2) {
			if(m1) {
				if(m2) {
					return maybe<A>(monoid<A>::append(*m1, *m2));
				}

				else {
					return m1;
				}
			}
			else if(m2) {
				return m2;
			}

			else {
				return maybe<A>();
			}
		}
	};

	template<typename A>
	maybe<A> operator^ (const maybe<A>& m1, const maybe<A>& m2) {
		return monoid<maybe<A>>::append(m1, m2);
	}

	/**
	 * Implementation of monad::bind for maybe.
	 */
	template<
		typename F,
		typename A,
		typename B = typename decayed_result<F(A)>::type>
	maybe<B> bind(const maybe<A>& m, const F& f) {
		if(m) {
			return value(f(*m));
		}

		return maybe<B>();
	}
}

#endif
