/**
   The detection idiom allows to apply different C++ code depending on
   if a template type has or does not have some signature.

   It is nicely described in this blog.
   
   https://blog.tartanllama.xyz/detection-idiom/

   This is not-quite-officially-supported in C++ 17 under
   std::experimental so until WCT moves to C++20 we explicitly provide
   a subset of this functionality.

   See test_detected.cxx for example usage.
 */


#ifndef WIRECELLUTIL_DETECTIONIDIOM
#define WIRECELLUTIL_DETECTIONIDIOM

#include <type_traits>          // false_type, etc

namespace WireCell {

    namespace detail {

        template <class... Ts>
        using void_t = void;

        template <class Default, class AlwaysVoid,
                  template<class...> class Op, class... Args>
        struct detector
        {
            using value_t = std::false_type;
            using type = Default;
        };

        template <class Default, template<class...> class Op, class... Args>
        struct detector<Default, std::void_t<Op<Args...>>, Op, Args...>
        {
            using value_t = std::true_type;
            using type = Op<Args...>;
        };

    } // namespace detail

    // special type to indicate detection failure
    struct nonesuch {
        nonesuch() = delete;
        ~nonesuch() = delete;
        nonesuch(nonesuch const&) = delete;
        void operator=(nonesuch const&) = delete;
    };

    template <template<class...> class Op, class... Args>
    using is_detected =
        typename detail::detector<nonesuch, void, Op, Args...>::value_t;

    template <template<class...> class Op, class... Args>
    using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

    template <class Default, template<class...> class Op, class... Args>
    using detected_or = detail::detector<Default, void, Op, Args...>;


}

#endif
