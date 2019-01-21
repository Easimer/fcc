#include "common.h"
#include <iostream>
#include <stack>
#include <cstdarg>
#include "shunting_yard.h"

inline bool whitespace(char c) {
	return
		c == ' ' ||
		c == '\t' ||
		c == '\n' ||
		c == '\r';
}

inline bool alpha(char c) {
	return
		(c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z');
}

inline bool digit(char c) {
	return c >= '0' && c <= '9';
}

inline bool alphanumeric(char c) {
	return alpha(c) || digit(c);
}

inline bool allowed_in_name(char c) {
	return
		alphanumeric(c) ||
		c == '_';
}

inline bool is_operator(char c) {
	return
		c == '+' or
		c == '-' or
		c == '*' or
		c == '/' or
		c == '=' or
		c == '<' or
		c == '>';
}

enum class token_nature {
	// Basic
	none, name, num_const, op, paren, arrow, comma, eol, colon,
	// Keywords
	declarator, definer, qextern,

	function, when
};

struct token {
	string str;
	token_nature nat;
	size_t row, col;
	token() {
		str.reserve(32);
		nat = token_nature::none;
	}

	token(const token& other) : str(other.str), nat(other.nat) {
	}

	token(token&& other) : str(std::move(other.str)), nat(other.nat) {
		other.nat = token_nature::none;
		other.str = string();
		other.str.reserve(32);
	}
};

struct fn_def {
};

struct fn_decl {
	string name;
	string type_ret;
	vector<string> type_args;
	bool q_extern;

	vector<fn_def> defs;
};

void tokenizer_emit_diag(size_t line, size_t col, const char* fmt, ...) {
	va_list va;
	fprintf(stderr, "Diagnostic on line %zu column %zu: ", line + 1, col + 1);
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, "\n");
}

const char* token_nature_str(token_nature nat) {
	switch(nat) {
#define TOKNAME(tok) case token_nature::tok: return #tok ;
		TOKNAME(none);
		TOKNAME(name);
		TOKNAME(num_const);
		TOKNAME(op);
		TOKNAME(paren);
		TOKNAME(arrow);
		TOKNAME(comma);
		TOKNAME(eol);
		TOKNAME(declarator);
		TOKNAME(definer);
		TOKNAME(qextern);
		TOKNAME(colon);
		TOKNAME(function);
		TOKNAME(when);
		default: return "unknown";
	}
}

std::ostream& operator<<(std::ostream& os, const token& tok) {
	os << "token(\"" << tok.str << "\" of " << token_nature_str(tok.nat) << " nature)";
	return os;
}

vector<token> tokenize(const string& expr) {
	vector<token> ret;
	token tok;

	size_t row = 0, col = 0;

	for(size_t i = 0; i < expr.size(); i++, col++) {
		auto c = expr[i];
		if(tok.nat == token_nature::none) {
			if(alpha(c)) {
				tok.str.push_back(c);
				tok.row = row; tok.col = col;
				tok.nat = token_nature::name;
			} else if(digit(c)) {
				tok.str.push_back(c);
				tok.row = row; tok.col = col;
				tok.nat = token_nature::num_const;
			} else if(whitespace(c)) {
				if(c == '\n') {
					col = 0;
					row++;
				}
				continue;
			} else if(c == '(' || c == ')') {
				tok.str.push_back(c);
				tok.row = row; tok.col = col;
				tok.nat = token_nature::paren;
				ret.push_back(std::move(tok));
			} else if(c == ',') {
				tok.str.push_back(c);
				tok.row = row; tok.col = col;
				tok.nat = token_nature::comma;
				ret.push_back(std::move(tok));
			} else if(c == '-') {
				if(i + 1 < expr.size() && expr[i + 1] == '>') {
					tok.str.push_back('-');
					tok.str.push_back('>');
					tok.row = row; tok.col = col;
					tok.nat = token_nature::arrow;
					ret.push_back(std::move(tok));
					i++, col++;
				} else {
					tok.str.push_back('-');
					tok.row = row; tok.col = col;
					tok.nat = token_nature::op;
					ret.push_back(std::move(tok));
				}
			} else if(is_operator(c)) {
				tok.str.push_back(c);
				tok.row = row; tok.col = col;
				tok.nat = token_nature::op;
				ret.push_back(std::move(tok));
			} else if(c == ';') {
				tok.nat = token_nature::eol;
				tok.row = row; tok.col = col;
				ret.push_back(std::move(tok));
			} else if(c == ':') {
				tok.nat = token_nature::colon;
				tok.row = row; tok.col = col;
				ret.push_back(std::move(tok));
			}
		} else {
			if(whitespace(c)) {
				ret.push_back(std::move(tok));
				if(c == '\n') {
					col = 0;
					row++;
				}
				continue;
			}
			if(tok.nat == token_nature::name) {
				if(allowed_in_name(c)) {
					tok.str.push_back(c);
				} else {
					ret.push_back(std::move(tok));
					i = i - 1;
				}
			}
			if(tok.nat == token_nature::num_const) {
				if(digit(c)) {
					tok.str.push_back(c);
				} else {
					ret.push_back(std::move(tok));
					i = i - 1;
				}
			}
		}
	}
	return ret;
}

vector<token> keyword_pass(vector<token>& tokens) {
	vector<token> ret;
	ret.reserve(tokens.size());
	for(size_t i = 0; i < tokens.size(); i++) {
		auto& token = tokens[i];
		if(token.nat == token_nature::name) {
			if(token.str == "decl") {
				token.nat = token_nature::declarator;
			} else if(token.str == "def") {
				token.nat = token_nature::definer;
			} else if(token.str == "when") {
				token.nat = token_nature::when;
			} else if(token.str == "extern") {
				if(i - 1 < ret.size()) {
					auto& prev = ret[i - 1];
					if(prev.nat == token_nature::declarator) {
						token.nat = token_nature::qextern;
					} else {
						tokenizer_emit_diag(token.row, token.col, "extern qualifier is only allowed in function declarations, ignored (prevtoken nature: %s)", token_nature_str(prev.nat));
						continue;
					}
				} else {
						tokenizer_emit_diag(token.row, token.col, "extern qualifier must come after declarator, ignored");
						continue;
				}
			} else {
				if(i + 1 < tokens.size()) {
					auto& ntoken = tokens[i + 1];
					if(ntoken.nat == token_nature::paren) {
						token.nat = token_nature::function;
					}
				}
			}
		}
		ret.push_back(std::move(token));
	}
	return ret;
}

vector<vector<token>> break_lines(vector<token>& tokens) {
	vector<vector<token>> ret;
	vector<token> cur;
	for(auto& tok : tokens) {
		if(tok.nat == token_nature::eol) {
			cur.push_back(std::move(tok));
			ret.push_back(std::move(cur));
		} else {
			cur.push_back(std::move(tok));
		}
	}
	return ret;
}

vector<fn_decl> fetch_fn_decls(const vector<vector<token>>& lines) {
	vector<fn_decl> ret;
	for(auto& line : lines) {
		if(line.size() == 0) {
			continue;
		}
		if(line[0].nat == token_nature::declarator) {
			fn_decl decl;
			// FSM 
			enum ccs {
				ccs_decl = 0, ccs_qual = 1, ccs_name = 2, ccs_col = 3, ccs_arg = 4,
				ccs_comma = 5, ccs_arrow = 6, ccs_type = 7, ccs_eol = 8,
			};
			ccs state = ccs_decl;
			size_t i = 0;
			bool is_declaration = true;
			while(i < line.size() && is_declaration) {
				auto toknat = line[i].nat;
				switch(state) {
					case ccs_decl: {
						if(toknat == token_nature::declarator) {
							i++;
							is_declaration = true;
							if(line[i].nat == token_nature::qextern) {
								state = ccs_qual;
							} else if(line[i].nat == token_nature::name) {
								state = ccs_name;
							} else {
								tokenizer_emit_diag(line[i].row, line[i].col, "expected qualifier or name in function declaration (got %s)", token_nature_str(line[i].nat));
								is_declaration = false;
							}
						} else {
							is_declaration = false;
						}
						break;
					}
					case ccs_qual: {
						if(toknat == token_nature::qextern) {
							// mark function extern
							decl.q_extern = true;
							i++;
							state = ccs_name;
						} else {
							tokenizer_emit_diag(line[i].row, line[i].col, "expected 'extern' in function declaration (got %s)", token_nature_str(toknat));
							is_declaration = false;
							break;
						}
						break;
					}
					case ccs_name: {
						if(toknat == token_nature::name) {
							// set fn name
							if(decl.name.size() < 0) {
								tokenizer_emit_diag(line[i].row, line[i].col, "function name cannot be empty");
								is_declaration = false;
								break;
							}
							decl.name = line[i].str;
							i++;
							state = ccs_col;
						} else {
							tokenizer_emit_diag(line[i].row, line[i].col, "expected function name in function declaration (got %s)", token_nature_str(toknat));
							is_declaration = false;
							break;
						}
						break;
					}
					case ccs_col: {
						if(toknat == token_nature::colon) {
							i++;
							state = ccs_arg;
						} else {
							tokenizer_emit_diag(line[i].row, line[i].col, "expected colon after function name in function declaration (got %s)", token_nature_str(toknat));
							is_declaration = false;
							break;
						}
						break;
					}
					case ccs_arg: {
						if(toknat == token_nature::name) {
							// append arg type
							decl.type_args.push_back(line[i].str);
							i++;
							state = ccs_comma;
							if(line[i].nat == token_nature::comma) {
								state = ccs_comma;
							} else if(line[i].nat == token_nature::arrow) {
								state = ccs_arrow;
							} else {
								tokenizer_emit_diag(line[i].row, line[i].col, "expected comma or arrow after function parameter in function declaration (got %s)", token_nature_str(line[i].nat));
								is_declaration = false;
							}
						} else {
							tokenizer_emit_diag(line[i].row, line[i].col, "expected function parameter in function declaration (got %s)", token_nature_str(toknat));
							is_declaration = false;
							break;
						}
						break;
					}
					case ccs_comma: {
						if(toknat == token_nature::comma) {
							i++;
							state = ccs_arg;
						}
						break;
					}
					case ccs_arrow: {
						if(toknat == token_nature::arrow) {
							i++;
							state = ccs_type;
						} else {
							tokenizer_emit_diag(line[i].row, line[i].col, "expected arrow after function parameter list in function declaration (got %s)", token_nature_str(toknat));
							is_declaration = false;
						}
						break;
					}
					case ccs_type: {
						if(toknat == token_nature::name) {
							// Set return type
							decl.type_ret = line[i].str;
							i++;
							state = ccs_eol;
						} else {
							tokenizer_emit_diag(line[i].row, line[i].col, "expected return type after arrow in function declaration (got %s)", token_nature_str(toknat));
							is_declaration = false;
						}
						break;
					}
					case ccs_eol: {
						if(toknat == token_nature::eol) {
							is_declaration = true;
							i++;
						} else {
							tokenizer_emit_diag(line[i].row, line[i].col, "expected end of line after return type in function declaration (got %s)", token_nature_str(toknat));
							is_declaration = false;
						}
						break;
					}
				}
			}
			if(is_declaration) {
				ret.push_back(std::move(decl));
			} else {
				tokenizer_emit_diag(line[0].row, line[0].col, "not a valid function declaration, ignored!!!");
			}
		}
	}
	return ret;
}

vector<sh_token> sh_genvector(const token* first, const token* last) {
	vector<sh_token> ret;

	while(first != last) {
		auto& tok = *first;
		sh_token shtok;
		shtok.str = tok.str;
		shtok.row = tok.row;
		shtok.col = tok.col;
		if(tok.nat == token_nature::name || tok.nat == token_nature::num_const) {
			shtok.type = sh_token_t::var;
			ret.push_back(std::move(shtok));
		} else if(tok.nat == token_nature::function) {
			shtok.type = sh_token_t::fun;
			ret.push_back(std::move(shtok));
		} else if(tok.nat == token_nature::paren) {
			if(shtok.str == "(") {
				shtok.type = sh_token_t::lbra;
			} else {
				shtok.type = sh_token_t::rbra;
			}
			ret.push_back(std::move(shtok));
		} else if(tok.nat == token_nature::op) {
			shtok.type = sh_token_t::op;
			ret.push_back(std::move(shtok));
		}
		first++;
	}

	return ret;
}

int op_prec(char c) {
#define OP_PREC(c, n) case c: return n
	switch(c) {
		OP_PREC('*', 5);
		OP_PREC('/', 5);
		OP_PREC('+', 6);
		OP_PREC('-', 6);
		OP_PREC('>', 9);
		OP_PREC('<', 9);
		OP_PREC('=', 10);
	}
	return -1;
}

struct expression;

struct expression {
	string str;
	expression *lhs, *rhs;
};

vector<vector<token>> break_expression(const token* first, const token* last) {
	vector<vector<token>> ret;
	vector<token> cur;

	while(first != last) {
		if(first->nat == token_nature::comma) {
			ret.push_back(std::move(cur));
		} else {
			cur.push_back(*first);
		}
		first++;
	}

	if(cur.size()) {
		ret.push_back(std::move(cur));
	}

	return ret;
}

vector<fn_def> fetch_fn_defs(const vector<vector<token>>& lines, const vector<fn_decl>& decls) {
	vector<fn_def> ret;

	for(size_t i = 0; i < lines.size(); i++) {
		auto& line = lines[i];
		if(line.size() > 0 && line[0].nat == token_nature::definer) {
			fn_def def;

			if(line[1].nat != token_nature::function) {
				tokenizer_emit_diag(line[1].row, line[1].col, "expected function name after definer");
				continue;
			}

			auto name = line[1].str;

			// lookup matching decl
			size_t decli = -1;
			for(size_t decl = 0; decl < decls.size(); decl++) {
				if(decls[decl].name == name) {
					decli = decl;
					break;
				}
			}
			if(decli == (size_t)-1) {
				tokenizer_emit_diag(line[1].row, line[1].col, "function defined, but not declared");
				continue;
			}

			auto& decl = decls[decli];

			// TODO: check if definition matches declaration
			size_t when_clause = 0;
			size_t arrow = 0;
			size_t eol = 0;

			for(size_t toki = 2; toki < line.size(); toki++) {
				if(line[toki].nat == token_nature::when) {
					when_clause = toki;
				}
				if(line[toki].nat == token_nature::arrow) {
					arrow = toki;
				}
				if(line[toki].nat == token_nature::eol) {
					eol = toki;
				}
			}

			if(!when_clause) {
				std::cerr << "(no when clause => base case)" << std::endl;
			}
			if(!arrow) {
				tokenizer_emit_diag(line[0].row, line[0].col, "expected arrow in function definition");
				continue;
			}
			if(!eol) {
				tokenizer_emit_diag(line[0].row, line[0].col, "WTF: no end of line");
				continue;
			}

			if(when_clause) {
				auto wcl_tokens = sh_genvector(line.data() + when_clause + 1, line.data() + arrow);

				wcl_tokens = shunting_yard(std::move(wcl_tokens), &op_prec);

				for(size_t i = 0; i < wcl_tokens.size(); i++) {
					std::cerr << wcl_tokens[i].str << ' ';
				}
				std::cerr << std::endl;
			}

			// break expression into subexpressions
			auto exprs = break_expression(line.data() + arrow + 1, line.data() + eol);

			for(auto& expr : exprs) {
				std::cerr << "EXPR: ";
				for(auto& tok : expr) {
					std::cerr << tok.str << ' ';
				}
				std::cerr << std::endl;

				auto expr_tokens = sh_genvector(expr.data(), expr.data() + expr.size());
				expr_tokens = shunting_yard(std::move(expr_tokens), &op_prec);
			}

		}
	}

	return ret;
}

int main(int argc, char** argv) {
	auto tokens = tokenize("decl fib : Z -> Z; def fib(n) when n = 1 -> 1; def fib(n) when n = 2 -> 1; def fib(n) -> fib(n - 2) + fib(n - 1);");
	tokens = keyword_pass(tokens);
	auto lines = break_lines(tokens);
	for(auto& line : lines) {
		for(auto& token : line) {
			std::cout << token << ' ';
		}
		std::cout << std::endl << std::endl;
	}
	auto fn_decls = fetch_fn_decls(lines);
	for(auto& decl : fn_decls) {
		std::cerr << decl.type_ret << ' ' << decl.name << '(';
		for(auto& arg : decl.type_args) {
			std::cerr << arg << ',';
		}
		std::cerr << ')' << std::endl;
	}

	auto fn_defs = fetch_fn_defs(lines, fn_decls);
	for(auto& def : fn_defs) {
		std::cerr << "def" << std::endl;
	}
	return 0;
}
