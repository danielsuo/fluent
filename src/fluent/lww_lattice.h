#ifndef FLUENT_LWW_LATTICE_H_
#define FLUENT_LWW_LATTICE_H_

#include "fluent/base_lattice.h"

namespace fluent {

template <typename L, typename R>
class LwwLattice : public Lattice<LwwLattice<L, R>, std::pair<L, R>> {
public:
	LwwLattice() = default;
	LwwLattice(const std::string &name) : name_(name), element_() {}
	LwwLattice(const std::pair<L, R> &e) : name_(""), element_(e) {}
	LwwLattice(const std::string &name, const std::pair<L, R> &e) : name_(name), element_(e) {}
	LwwLattice(const LwwLattice<L, R> &l) = default;
	LwwLattice& operator=(const LwwLattice<L, R>& l) = default;

	const std::string& Name() const override { return name_; }
	const std::pair<L, R>& Reveal() const override { return element_; }
  	void merge(const LwwLattice<L, R>& l) override {
  		if (std::get<0>(element_) < std::get<0>(l.element_)) {
  			std::get<0>(element_) = std::get<0>(l.element_);
  			std::get<1>(element_) = std::get<1>(l.element_);
  		}
  	}
  	void merge(const std::pair<L, R>& t) override {
  		if (std::get<0>(element_) < std::get<0>(t)) {
  			std::get<0>(element_) = std::get<0>(t);
  			std::get<1>(element_) = std::get<1>(t);
  		}
  	}

	template <typename RA>
	typename std::enable_if<!(std::is_base_of<Lattice<LwwLattice<L, R>, std::pair<L, R>>, RA>::value)>::type
	Merge(const RA& ra) {
		auto buf = ra::MergeRaInto<RA>(ra);
		auto begin = std::make_move_iterator(std::begin(buf));
		auto end = std::make_move_iterator(std::end(buf));
		for (auto it = begin; it != end; it++) {
		  merge(std::get<0>(*it));
		}
	}

	template <typename S>
	typename std::enable_if<(std::is_base_of<Lattice<LwwLattice<L, R>, std::pair<L, R>>, S>::value)>::type
	Merge(const S& l) {
		merge(l);
	}

	friend bool operator==(const LwwLattice<L, R>& lhs, const LwwLattice<L, R>& rhs) {
		return lhs.element_ == rhs.element_;
	}
	friend bool operator!=(const LwwLattice<L, R>& lhs, const LwwLattice<L, R>& rhs) {
		return lhs.element_ != rhs.element_;
	}

private:
	std::string name_;
	std::pair<L, R> element_;
};

}  // namespace fluent

#endif  // FLUENT_LWW_LATTICE_H_