
#include <assert.h>
#include <map>
#include "json.h"

class Error {
protected:
	std::string msg;
public:
	virtual std::string message() const
	{
		return msg;
	}
};

class SyntaxError : public Error {
public:
	SyntaxError()
	{
		msg = "Syntax error.";
	}
};

class UnknownOperator : public Error {
public:
	UnknownOperator(std::string const &name)
	{
		msg = "Unknown operator '";
		msg += name;
		msg += "'.";
	}
};

class ArgumentCountIncorrect : public Error {
public:
	ArgumentCountIncorrect()
	{
		msg = "Argument count incorrect.";
	}
};

class VariableNotFound : public Error {
public:
	VariableNotFound(std::string const &name)
	{
		msg = "Variable not found '";
		msg += name;
		msg += "'.";
	}
};

class OreLang {
public:
private:

	class Value {
	private:
		double d;
	public:
		void set(double v)
		{
			d = v;
		}
		double to_number() const
		{
			return d;
		}
		bool to_bool() const
		{
			return d != 0;
		}
		std::string to_string() const
		{
			char tmp[1000];
			sprintf(tmp, "%f", d);
			return tmp;
		}
		bool operator <= (Value const &r) const
		{
			return d <= r.d;
		}
		Value operator + (Value const &r) const
		{
			Value v;
			v.d = d + r.d;
			return v;
		}
	};

	std::map<std::string, Value> vars;

	bool getvar(std::string const &name, Value *result)
	{
		auto it = vars.find(name);
		if (it != vars.end()) {
			*result = it->second;
			return true;
		}
		throw VariableNotFound(name);
	}

	void eval(JSON::Node const &node, Value *result)
	{
		assert(result);
		switch (node.type) {
		case JSON::Type::Array:
			run(node.children, 0, result);
			break;
		case JSON::Type::String:
			getvar(node.value, result);
			break;
		case JSON::Type::Number:
		case JSON::Type::Boolean:
			*result = Value();
			result->set(strtod(node.value.c_str(), nullptr));
			break;
		}
	}

	size_t run(std::vector<JSON::Node> const &program, size_t position, Value *result = nullptr)
	{
		size_t pos = position;
		while (pos < program.size()) {
			JSON::Node const &node = program[pos];
			if (node.type == JSON::Type::String) {
				std::string op = node.value.c_str();
				if (op == "step") {
					pos++;
					pos += run(program, pos);
				} else if (op == "set") {
					if (program.size() != 3) throw ArgumentCountIncorrect();
					std::string name = program[1].value;
					Value v;
					eval(program[2], &v);
					vars[name] = v;
					pos += 3;
				} else if (op == "get") {
					assert(result);
					if (program.size() != 2) throw ArgumentCountIncorrect();
					eval(program[1], result);
					pos += 2;
				} else if (op == "while") {
					if (program.size() != 3) throw ArgumentCountIncorrect();
					while (1) {
						Value ret;
						run(program[1].children, 0, &ret);
						if (!ret.to_bool()) break;
						run(program[2].children, 0);
					}
					pos += 3;
				} else if (op == "<=") {
					assert(result);
					if (program.size() != 3) throw ArgumentCountIncorrect();
					Value lv, rv;
					eval(program[1], &lv);
					eval(program[2], &rv);
					result->set(lv <= rv);
					pos += 3;
				} else if (op == "+") {
					assert(result);
					if (program.size() != 3) throw ArgumentCountIncorrect();
					Value lv, rv;
					eval(program[1], &lv);
					eval(program[2], &rv);
					*result = lv + rv;
					pos += 3;
				} else if (op == "print") {
					if (program.size() != 2) throw ArgumentCountIncorrect();
					Value v;
					eval(program[1], &v);
					std::string s = v.to_string();
					puts(s.c_str());
					pos += 2;
				} else {
					throw UnknownOperator(op);
				}
			} else if (node.type == JSON::Type::Array) { // [...]
				run(node.children, 0);
				pos++;
			}
		}
		return pos - position;
	}
public:
	void run(JSON const &json)
	{
		run(json.node.children, 0);
	}
};

int main(int argc, char **argv)
{
	static char const source[] =
		"[\"step\","
		"  [\"set\", \"sum\", 0 ],"
		"  [\"set\", \"i\", 1 ],"
		"  [\"while\", [\"<=\", [\"get\", \"i\"], 10],"
		"    [\"step\","
		"      [\"set\", \"sum\", [\"+\", [\"get\", \"sum\"], [\"get\", \"i\"]]],"
		"      [\"set\", \"i\", [\"+\", [\"get\", \"i\"], 1]]]],"
		"  [\"print\", [\"get\", \"sum\"]]]\"";

	try {
		JSON json;
		bool f = json.parse(source);
		if (!f) throw SyntaxError();
		OreLang orelang;
		orelang.run(json);
	} catch (Error &e) {
		fprintf(stderr, "error: %s\n", e.message().c_str());
	}

	return 0;
}
