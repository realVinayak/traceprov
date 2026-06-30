#ifndef __TP_EXPRNS__
#define __TP_EXPRNS__
#include "duckdb.hpp"
using namespace duckdb;
class TraceProvBoundComparison : public Expression {
public:
	static constexpr const ExpressionClass TYPE = ExpressionClass::BOUND_COMPARISON;

public:
	TraceProvBoundComparison(ExpressionType type, unique_ptr<Expression> left, unique_ptr<Expression> right);

	unique_ptr<Expression> left;
	unique_ptr<Expression> right;

public:
	string ToString() const override;

	bool Equals(const BaseExpression &other) const override;

	// unique_ptr<Expression> Copy() override;

	void Serialize(Serializer &serializer) const override;
	static unique_ptr<Expression> Deserialize(Deserializer &deserializer);

public:
	static LogicalType BindComparison(ClientContext &context, const LogicalType &left_type,
	                                  const LogicalType &right_type, ExpressionType comparison_type);
	static bool TryBindComparison(ClientContext &context, const LogicalType &left_type, const LogicalType &right_type,
	                              LogicalType &result_type, ExpressionType comparison_type);
};

class TraceProvBoundConstantExpression : public Expression {
public:
	static constexpr const ExpressionClass TYPE = ExpressionClass::BOUND_CONSTANT;

public:
	explicit TraceProvBoundConstantExpression(Value value);

	Value value;

public:
	string ToString() const override;

	bool Equals(const BaseExpression &other) const override;
	hash_t Hash() const override;

	// unique_ptr<Expression> Copy() override;

	void Serialize(Serializer &serializer) const override;
	static unique_ptr<Expression> Deserialize(Deserializer &deserializer);
};

class TraceProvBoundBetweenExpression : public Expression {
public:
	static constexpr const ExpressionClass TYPE = ExpressionClass::BOUND_BETWEEN;

public:
	TraceProvBoundBetweenExpression(unique_ptr<Expression> input, unique_ptr<Expression> lower, unique_ptr<Expression> upper,
	                       bool lower_inclusive, bool upper_inclusive);

	unique_ptr<Expression> input;
	unique_ptr<Expression> lower;
	unique_ptr<Expression> upper;
	bool lower_inclusive;
	bool upper_inclusive;

public:
	string ToString() const override;

	bool Equals(const BaseExpression &other) const override;

	// unique_ptr<Expression> Copy() override;

	void Serialize(Serializer &serializer) const override;
	static unique_ptr<Expression> Deserialize(Deserializer &deserializer);

public:
	ExpressionType LowerComparisonType() {
		return lower_inclusive ? ExpressionType::COMPARE_GREATERTHANOREQUALTO : ExpressionType::COMPARE_GREATERTHAN;
	}
	ExpressionType UpperComparisonType() {
		return upper_inclusive ? ExpressionType::COMPARE_LESSTHANOREQUALTO : ExpressionType::COMPARE_LESSTHAN;
	}

private:
	TraceProvBoundBetweenExpression();
};

#endif