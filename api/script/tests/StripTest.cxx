/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/script/Strip.h>

#include <cstddef>
#include <string>
#include <string_view>

#include <boost/test/unit_test.hpp>

namespace {

/**
 * Check that a source strips to some JavaScript, which keeps its length and its lines. Each
 * expected text is what Node's own type stripper makes of the same source.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the source, then what it strips to
void stripsTo(std::string_view source, std::string_view expected) {
    const v3d::script::Stripped stripped = v3d::script::strip(source);
    const std::string why = stripped.problem ? stripped.problem->message : std::string();
    BOOST_REQUIRE_MESSAGE(!stripped.problem, why);
    BOOST_CHECK_EQUAL(stripped.code, expected);
    BOOST_CHECK_EQUAL(stripped.code.size(), source.size());
}

/**
 * Check that a source is refused at a line and column, for a reason that begins some way.
 **/
void refused(std::string_view source, std::size_t line, std::size_t column, std::string_view begins) {
    const v3d::script::Stripped stripped = v3d::script::strip(source);
    BOOST_CHECK(stripped.problem);
    BOOST_CHECK(stripped.code.empty());
    const v3d::script::Problem problem = stripped.problem.value_or(v3d::script::Problem{});
    BOOST_CHECK_EQUAL(problem.line, line);
    BOOST_CHECK_EQUAL(problem.column, column);
    BOOST_CHECK_MESSAGE(problem.message.starts_with(begins), problem.message);
}

BOOST_AUTO_TEST_SUITE(script_strip_test)

/**
 * Annotations on variables, parameters and returns are blanked, with an optional mark and a
 * definite assignment's !.
 **/
BOOST_AUTO_TEST_CASE(annotations_are_blanked) {
    stripsTo(R"ts(let a: number = 1, b: string | undefined;
var c!: Array<{ x: number; y: string }>;
function f(x: number, y?: string, ...rest: number[]): void {}
const g = (x: number): number => x * 2;
try {} catch (e: unknown) {})ts",
        R"ts(let a         = 1, b                    ;
var c                                  ;
function f(x        , y         , ...rest          )       {}
const g = (x        )         => x * 2;
try {} catch (e         ) {})ts");
}

/**
 * Type parameters are blanked, and so are type arguments, which a comparison is told apart from.
 **/
BOOST_AUTO_TEST_CASE(type_parameters_and_arguments_are_blanked) {
    stripsTo(R"ts(function id<T extends object = {}>(this: Window, value: T): T { return value; }
const h = <T,>(x: T): T => x;
const m = new Map<string, Array<Set<number>>>();
const call = make<string>("a"), inst = f<number>;
const less = a < b && c > d, shift = a >> 2, ge = a >= b;
const g4 = f < T > (x);)ts",
        R"ts(function id                       (              value   )    { return value; }
const h =     (x   )    => x;
const m = new Map                            ();
const call = make        ("a"), inst = f        ;
const less = a < b && c > d, shift = a >> 2, ge = a >= b;
const g4 = f       (x);)ts");
}

/**
 * as, satisfies and a non-null assertion are blanked, and so is as const.
 **/
BOOST_AUTO_TEST_CASE(assertions_are_blanked) {
    stripsTo(R"ts(const x = y as unknown as Record<string, () => void>;
const z = (w as string).length, t = [1, 2] as const;
const cfg = { a: 1 } satisfies Config;
const u = q!.r!.s, v = a?.b!.c?.[d]!?.(e)!;
if (x! > 0) {})ts",
        R"ts(const x = y                                         ;
const z = (w          ).length, t = [1, 2]         ;
const cfg = { a: 1 }                 ;
const u = q .r .s, v = a?.b .c?.[d] ?.(e) ;
if (x  > 0) {})ts");
}

/**
 * An interface, a type alias, an ambient declaration, an overload and a type-only import or export
 * are blanked whole.
 **/
BOOST_AUTO_TEST_CASE(declarations_of_types_are_blanked) {
    stripsTo(R"ts(interface Point { x: number; y: number }
type Alias<T> = { value: T } | null;
declare const d: number;
declare namespace NS { const y: number; }
declare global { interface Window { v: number } }
function over(x: string): string;
function over(x: any) { return x; }
import type { Something } from "./somewhere";
import { type A, B } from "./mod";
export type { Point };
let type = 1;
type = 2;)ts",
        R"ts(                                        
                                    
                        
                                         
                                                 
                                 
function over(x     ) { return x; }
                                             
import {         B } from "./mod";
                      
let type = 1;
type = 2;)ts");
}

/**
 * A class loses its modifiers, its implements clause, and its members that are only types.
 **/
BOOST_AUTO_TEST_CASE(class_members_lose_their_types) {
    stripsTo(R"ts(abstract class Base<T> extends Root<T> implements I, J<T> {
  private x: number = 1;
  protected readonly y?: string;
  declare w: number;
  abstract m(): void;
  [key: string]: any;
  q!: number;
  overloaded(a: string): void;
  overloaded(a: any) {}
  get value(): number { return this.x; }
  method<U>(this: Base<T>, u: U): U { return u; }
})ts",
        R"ts(         class Base    extends Root                       {
          x         = 1;
                     y         ;
                    
                     
                     
  q         ;
                              
  overloaded(a     ) {}
  get value()         { return this.x; }
  method   (               u   )    { return u; }
})ts");
}

/**
 * A modifier erased before a computed name or a generator leaves a semicolon, so a field before it
 * without one does not run on into it.
 **/
BOOST_AUTO_TEST_CASE(a_modifier_before_a_computed_member_leaves_a_semicolon) {
    stripsTo(R"ts(class A {
  x = 1
  public [k]() {}
  y = 2
  public async *g() {}
  z = 3
  public static [j] = 1
})ts",
        R"ts(class A {
  x = 1
  ;      [k]() {}
  y = 2
  ;      async *g() {}
  z = 3
         static [j] = 1
})ts");
}

/**
 * A return type that runs onto another line takes the ) before it to its end, since => may not
 * follow a line break.
 **/
BOOST_AUTO_TEST_CASE(a_return_type_over_lines_takes_its_parenthesis) {
    stripsTo(R"ts(const h = (i: number)
  : number => i
const typed = (a): {
  x: number
} => ({ x: 1 });)ts",
        R"ts(const h = (i         
         ) => i
const typed = (a    
           
) => ({ x: 1 });)ts");
}

/**
 * In a conditional's first branch, a parenthesis then a colon is an arrow function only when the
 * conditional's own colon follows it.
 **/
BOOST_AUTO_TEST_CASE(a_conditional_tells_a_typed_arrow_from_its_branches) {
    stripsTo(R"ts(const cond = a ? (b) : c => d;
const cond2 = a ? (b): number => b : c;
const cond3 = a
  ? (b) : c)ts",
        R"ts(const cond = a ? (b) : c => d;
const cond2 = a ? (b)         => b : c;
const cond3 = a
  ? (b) : c)ts");
}

/**
 * Regular expressions, division, templates, labels and object literals pass through unchanged.
 **/
BOOST_AUTO_TEST_CASE(code_that_is_not_a_type_is_kept) {
    stripsTo(R"ts(const a1 = x / y / z, r1 = /[/]/.source, r2 = (/a/), r3 = [/b/g];
const s1 = `a ${b ? `c ${d as any}` : e} f`;
const obj = { m(x: number): string { return ""; }, get v(): number { return 1; }, [k]: 1, q, ...rest };
label: for (let i: number = 0; i < 10; i++) { continue label; }
let as = 1, satisfies = 2, kw = { new: 1, as: 5 };
/* x: y */ let k1 /* : not */ : number = 1;)ts",
        R"ts(const a1 = x / y / z, r1 = /[/]/.source, r2 = (/a/), r3 = [/b/g];
const s1 = `a ${b ? `c ${d       }` : e} f`;
const obj = { m(x        )         { return ""; }, get v()         { return 1; }, [k]: 1, q, ...rest };
label: for (let i         = 0; i < 10; i++) { continue label; }
let as = 1, satisfies = 2, kw = { new: 1, as: 5 };
/* x: y */ let k1 /* : not */          = 1;)ts");
}

/**
 * Types of every form are read to their end: functions, constructors, tuples, mapped, conditional,
 * template and indexed types, and predicates.
 **/
BOOST_AUTO_TEST_CASE(types_of_every_form_are_read_to_their_end) {
    stripsTo(R"ts(let f: (cb: (err: Error | null, data?: Buffer) => void) => Promise<void>;
let c: abstract new (...args: any[]) => object;
let t: [a: number, b?: string, ...rest: boolean[]];
let m: { readonly [P in K as `get${Capitalize<P>}`]-?: () => T[P] };
let k: T extends [infer H, ...infer R] ? H : T extends string ? "s" : "o";
let i: A["b"][number], n: -1 | 1 = 1, q: typeof a.b, im: import("./x").Y;
function guard(x: unknown): x is string { return typeof x === "string"; }
function check(x: unknown): asserts x is number {})ts",
        R"ts(let f                                                                   ;
let c                                         ;
let t                                             ;
let m                                                              ;
let k                                                                    ;
let i                , n         = 1, q            , im                 ;
function guard(x         )              { return typeof x === "string"; }
function check(x         )                      {})ts");
}

/**
 * Syntax with no JavaScript to keep when its types go is refused where it begins.
 **/
BOOST_AUTO_TEST_CASE(syntax_that_cannot_be_erased_is_refused) {
    refused("let a = 1;\nenum Color { Red }", 2, 1, "an enum cannot be erased");
    refused("const enum E { A }", 1, 1, "an enum cannot be erased");
    refused("namespace N { export const x = 1; }", 1, 1, "a namespace cannot be erased");
    refused("module M { }", 1, 1, "a namespace cannot be erased");
    refused("class P { constructor(private x: number) {} }", 1, 23, "a parameter property cannot be erased");
    refused("const y = <number>x;", 1, 11, "a type assertion written <T>value cannot be erased");
    refused("import fs = require(\"fs\");", 1, 8, "import = cannot be erased");
    refused("export = x;", 1, 8, "export = cannot be erased");
}

/**
 * A source that does not read is refused where reading stopped, and so is one with something
 * left open.
 **/
BOOST_AUTO_TEST_CASE(a_source_that_does_not_read_is_refused) {
    refused("let a = ;", 1, 9, "the script cannot be read here");
    refused("let a: = 1;", 1, 8, "the script cannot be read here");
    refused("f(a b);", 1, 5, "the script cannot be read here");
    refused("let a = 1;\n/* never closed", 2, 1, "a comment, string or template here is not closed");
    refused("let s = \"never closed\n", 1, 9, "a comment, string or template here is not closed");
    refused("let t = `${a}", 1, 9, "a template here is not closed");
}

/**
 * A source nested deeper than the parser reads is refused rather than read, and a nesting that
 * is read twice, as parameters and then as an expression, is read in time.
 **/
BOOST_AUTO_TEST_CASE(nesting_is_bounded) {
    const std::string deep = "let w = " + std::string(5000, '(') + "1" + std::string(5000, ')') + ";";
    refused(deep, 1, 264, "the script nests too deeply here");

    std::string defaults = "let v = ";
    for (int each = 0; each < 40; ++each) {
        defaults += "(a" + std::to_string(each) + " = ";
    }
    defaults += "1" + std::string(40, ')') + ";";
    const v3d::script::Stripped stripped = v3d::script::strip(defaults);
    BOOST_CHECK(!stripped.problem);
    BOOST_CHECK_EQUAL(stripped.code, defaults);
}

/**
 * Plain JavaScript passes through as it is, an interpreter line and a byte order mark included.
 **/
BOOST_AUTO_TEST_CASE(javascript_passes_through) {
    for (const char* source : { "", "\xEF\xBB\xBFlet a = 1;", "#!/usr/bin/env node\nlet b = a ? c : d;\n", "x\n++y\n" }) {
        stripsTo(source, source);
    }
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
