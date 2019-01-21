#include "common.h"
#include "shunting_yard.h"

vector<sh_token> shunting_yard(vector<sh_token>&& sh_tokens, operator_precedence_t prec) {
	vector<sh_token> s_ret;
	vector<sh_token> s_op;

	for(size_t i = 0; i < sh_tokens.size(); i++) {
		auto& token = sh_tokens[i];
		if(token.type == sh_token_t::var) {
			s_ret.push_back(std::move(token));
		}
		if(token.type == sh_token_t::fun) {
			s_op.push_back(std::move(token));
		}
		if(token.type == sh_token_t::op) {
			if(s_op.size() > 0) {
				auto precedence = prec(token.str[0]);
				auto& op_top = s_op.back();
				auto op_top_prec = prec(op_top.str[0]);
				while(
						((op_top.type == sh_token_t::fun) or
						(precedence >= op_top_prec)) and op_top.type != sh_token_t::lbra
 					)	{
					s_ret.push_back(std::move(op_top));
					s_op.pop_back();
					op_top = s_op.back();
					op_top_prec = prec(op_top.str[0]);
				}
			}
			s_op.push_back(std::move(token));
		}
		if(token.type == sh_token_t::lbra) {
			s_op.push_back(std::move(token));
		}
		if(token.type == sh_token_t::rbra) {
			auto& op_top = s_op.back();
			while(op_top.type != sh_token_t::lbra) {
				s_ret.push_back(std::move(op_top));
				s_op.pop_back();
				op_top = s_op.back();
			}
			s_op.pop_back();
		}
	}

	while(s_op.size()) {
		s_ret.push_back(std::move(s_op.back()));
		s_op.pop_back();
	}

	return s_ret;
}
