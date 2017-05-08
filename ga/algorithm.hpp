#ifndef SPEA2_ALGORITHM_HPP
#define SPEA2_ALGORITHM_HPP

#include <ga/meta.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <type_traits>
#include <vector>

namespace ga
{

template <typename G> static auto draw(double rate, G& g)
{
  return std::generate_canonical<double, std::numeric_limits<double>::digits>(g) < rate;
}

template <typename T, typename E = void> class algorithm
{
  static_assert(meta::always_false<T>::value,
                "Problem type doesn't complies with the required concept");
};

template <typename T> class algorithm<T, meta::requires<meta::Problem<T>>>
{
  using individual_type = typename T::individual_type;
  using generator_type = typename T::generator_type;
  using fitness_type = meta::evaluate_result<T>;

public:
  struct solution_type
  {
    individual_type x;
    fitness_type fitness;
  };

  algorithm(T problem, std::vector<individual_type> population, std::size_t elite_count,
            generator_type generator)
    : problem_(std::move(problem))
    , elite_count_(elite_count)
    , generator_(std::move(generator))
  {
    if (elite_count_ >= population.size())
      throw std::runtime_error{"elite count is greater than population size"};

    population_.reserve(population.size());
    next_population_.reserve(population.size() - elite_count_);

    std::transform(population.begin(), population.end(), back_inserter(population_),
                   [&](auto& x) -> solution_type {
                     const auto fitness = problem_.evaluate(x, generator_);
                     return {std::move(x), fitness};
                   });

    sort_population();
  }

  auto iterate() -> void
  {
    // == Mating Selection, Recombination and Mutation ==
    // We perform binary tournament selection with replacement.
    auto indexes =
      std::uniform_int_distribution<std::size_t>(0u, population_.size() - 1u);

    const auto binary_tournament = [&]() -> const individual_type& {
      const auto i = sample_from(indexes);
      const auto j = sample_from(indexes);
      return population_[i].fitness < population_[j].fitness ? population_[i].x
                                                             : population_[j].x;
    };

    while (next_population_.size() < population_.size() - elite_count_)
    {
      // Two binary tournament to select the parents.
      const auto& parent1 = binary_tournament();
      const auto& parent2 = binary_tournament();

      // Children are either a recombination or the parents themselves.
      auto children = problem_.recombine(parent1, parent2, generator_);

      // Mutate and put children in the new population.
      for (auto& child : children)
      {
        problem_.mutate(child, generator_);
        const auto fitness = problem_.evaluate(child, generator_);
        next_population_.push_back({std::move(child), fitness});
        if (next_population_.size() == population_.size() - elite_count_)
          break;
      }
    }

    // Copy new individual preserving the elite_count_ best ones.
    std::move(next_population_.begin(), next_population_.end(),
              population_.begin() + elite_count_);
    next_population_.clear();

    sort_population();
  }

  auto population() const -> const std::vector<solution_type>& { return population_; }

  auto problem() -> T& { return problem_; }
  auto problem() const -> const T& { return problem_; }

  auto generator() -> generator_type& { return generator_; }
  auto generator() const -> const generator_type& { return generator_; }

  auto elite_count() const { return elite_count_; }

private:
  auto sort_population()
  {
    std::sort(population_.begin(), population_.end(),
              [](const auto& a, const auto& b) { return a.fitness < b.fitness; });
  }

  template <typename Distribution> auto sample_from(Distribution& dist)
  {
    return dist(generator_);
  }

  T problem_;

  std::vector<solution_type> population_, next_population_;

  const std::size_t elite_count_;

  generator_type generator_;
};

template <typename T, typename I, typename G, typename = meta::requires<meta::Problem<T>>>
auto make_algorithm(T problem, std::vector<I> population, std::size_t elite_count,
                    G generator) -> algorithm<T>
{
  return {std::move(problem), std::move(population), elite_count, std::move(generator)};
}

template <typename T, typename... Args, typename = meta::fallback<meta::Problem<T>>>
auto make_algorithm(T, Args&&...)
{
  static_assert(meta::always_false<T>::value,
                "Problem type doesn't complies with the required concept");
}

} // namespace ga

#endif // SPEA2_ALGORITHM_HPP
