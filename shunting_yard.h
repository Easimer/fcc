#pragma once
#include "common.h"

using operator_precedence_t = int(*)(char);

enum class sh_token_t {
	var, fun, op,
	lbra, rbra,
};

struct sh_token {
	sh_token_t type;
	string str;
	size_t row, col;
};

vector<sh_token> shunting_yard(vector<sh_token>&& tokens, operator_precedence_t prec);
