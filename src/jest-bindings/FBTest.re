/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */;

module Types = {
  type matcherUtils = {tmp: unit};
  type matcherConfig('a, 'b) =
    (matcherUtils, unit => 'a, unit => 'b) => (unit => string, bool);
  type matcherResult('a, 'b) = (unit => 'a, unit => 'b) => unit;
  type createMatcher('a, 'b) =
    matcherConfig('a, 'b) => matcherResult('a, 'b);
  type extendUtils = {createMatcher: 'a 'b. createMatcher('a, 'b)};
  type stringUtils = {
    toBeEmpty: unit => unit,
    toEqual: string => unit,
  };
  type stringUtilsWithNot = {
    toBeEmpty: unit => unit,
    toEqual: string => unit,
    toMatchSnapshot: unit => unit,
    not: stringUtils,
  };
  type boolUtils = {
    toBe: bool => unit,
    toBeTrue: unit => unit,
    toBeFalse: unit => unit,
  };
  type boolUtilsWithNot = {
    toBe: bool => unit,
    toBeTrue: unit => unit,
    toBeFalse: unit => unit,
    not: boolUtils,
  };
  type intUtils = {toBe: int => unit};
  type intUtilsWithNot = {
    toBe: int => unit,
    not: intUtils,
  };
  type floatUtils = {toBeCloseTo: (~digits: int=?, float) => unit};
  type floatUtilsWithNot = {
    toBeCloseTo: (~digits: int=?, float) => unit,
    not: floatUtils,
  };
  type collectionKind('a) =
    | String: collectionKind(string)
    | Bool: collectionKind(bool)
    | Int: collectionKind(int)
    | Float: collectionKind(float)
    | Custom(('a, 'a) => bool): collectionKind('a);
  type listOfUtils('a) = {toEqual: list('a) => unit};
  type listOfUtilsWithNot('a) = {
    toEqual: list('a) => unit,
    not: listOfUtils('a),
  };
  type arrayOfUtils('a) = {toEqual: array('a) => unit};
  type arrayOfUtilsWithNot('a) = {
    toEqual: array('a) => unit,
    not: arrayOfUtils('a),
  };
  type expectUtils('ext) = {
    string: string => stringUtilsWithNot,
    bool: bool => boolUtilsWithNot,
    int: int => intUtilsWithNot,
    float: float => floatUtilsWithNot,
    listOf: 'a. (collectionKind('a), list('a)) => listOfUtilsWithNot('a),
    arrayOf: 'a. (collectionKind('a), array('a)) => arrayOfUtilsWithNot('a),
    ext: 'ext,
  };
  type testUtils('ext) = {
    expect: expectUtils('ext),
    expectTrue: bool => unit,
    expectFalse: bool => unit,
    expectEqual: 'a. ('a, 'a) => unit,
    expectNotEqual: 'a. ('a, 'a) => unit,
    expectSame: 'a. ('a, 'a) => unit,
    expectNotSame: 'a. ('a, 'a) => unit,
  };
  type test('ext) = (string, testUtils('ext) => unit) => unit;
  type describeUtils('ext) = {
    beforeEach: (unit => unit) => unit,
    beforeAll: (unit => unit) => unit,
    test: test('ext),
    testOnly: test('ext),
    testSkip: test('ext),
  };
  type describeFn('ext) = (string, describeUtils('ext) => unit) => unit;

  type extensionResult('ext) = {describe: describeFn('ext)};
};

open Types;

type undefined = Js.undefined(unit);

let undefined: undefined = Js.Undefined.empty;

/**
The underlying, external Jest implementations. These should not be used outside
of this file.
*/
module Jest = {
  [@bs.scope "jest"] [@bs.val] external resetModules: unit => undefined = "";
  [@bs.val] external describe: (string, unit => undefined) => undefined = "";
  [@bs.val] external beforeAll: (unit => undefined) => undefined = "";
  [@bs.val] external beforeEach: (unit => undefined) => undefined = "";
  [@bs.val] external test: (string, unit => undefined) => undefined = "";
  [@bs.scope "test"] [@bs.val]
  external testOnly: (string, unit => undefined) => undefined = "only";
  [@bs.scope "test"] [@bs.val]
  external testSkip: (string, unit => undefined) => undefined = "skip";
  type expectResult;
  [@bs.val] external expect: 'a => expectResult = "";
  [@bs.send] external toMatchSnapshot: expectResult => undefined = "";
  [@bs.send] external toBe: (expectResult, 'a) => undefined = "";
  [@bs.scope "not"] [@bs.send]
  external notToBe: (expectResult, 'a) => undefined = "toBe";
  [@bs.send] external toEqual: (expectResult, 'a) => undefined = "";
  [@bs.scope "not"] [@bs.send]
  external notToEqual: (expectResult, 'a) => undefined = "toEqual";
  [@bs.send] external toBeCloseTo: (expectResult, 'a, int) => undefined = "";
  [@bs.scope "not"] [@bs.send]
  external notToBeCloseTo: (expectResult, 'a, int) => undefined =
    "toBeCloseTo";
};

module Extensions = {
  type expectExtendResult = {
    .
    "message": (. unit) => string,
    "pass": bool,
  };
  type reasonRootMatcher('a, 'b) = {
    .
    "reasonRootMatcher": ('a, (string, 'b)) => expectExtendResult,
  };
  [@bs.val]
  external expectDotExtend: reasonRootMatcher('a, 'b) => unit =
    "expect.extend";
  [@bs.send]
  external expectReasonRootMatcher:
    (Jest.expectResult, (string, 'a)) => undefined =
    "reasonRootMatcher";
  /* The structure we need to construct to represent a matcher */
  type matcher('a, 'b) = {
    name: string,
    test: (unit => 'a, unit => 'b) => (unit => string, bool),
  };
  /* Probably equivalent to matcher.length > 0, but being explicit is safer. */
  let registeredRootMatcher = ref(false);
  /* TODO: Should use a map */
  let matchers = ref([]);
  let matcherCounter = Counter.create();
  /* Locates a matcher by name, may throw if the matcher is not found. */
  let findMatcherExn = name => {
    let result =
      matchers.contents
      |> List.fold_left(
           (acc, matcher) =>
             switch (acc, matcher.name == name) {
             | (_, false) => acc
             | (None, true) => Some(matcher)
             /* TODO: Better formatted error messages? */
             | (Some(_), true) =>
               failwith("Duplicate matchers created with name: " ++ name)
             },
           None,
         );
    switch (result) {
    | Some(matcher) => matcher
    /* TODO: Better formatted error messages? */
    | None => failwith("Error finding matcher with name: " ++ name)
    };
  };
  let maybeRegisterRootMatcher = () =>
    if (! registeredRootMatcher^) {
      registeredRootMatcher := true;
      expectDotExtend({
        "reasonRootMatcher": (actual, (name, expected)) => {
          let matcher = findMatcherExn(name);
          let (getMessage, didPass) =
            matcher.test(() => actual, () => expected);
          let getMessageJS = (.) => getMessage();
          let didPassJS = didPass;
          {"message": getMessageJS, "pass": didPassJS};
        },
      });
    };
  let matcherUtils = {tmp: ()};
  let maybeRegisterMatcher = (name, buildMatcher) => {
    /* Always check if we need to register the root matcher. */
    maybeRegisterRootMatcher();
    /* Register this specific matcher if necessary */
    if (!List.exists(m => m.name == name, matchers^)) {
      let matcherFn = buildMatcher(matcherUtils);
      let matcher = {name, test: matcherFn};
      matchers := matchers.contents @ [matcher];
    };
  };
  let safeCreateMatcher = buildMatcher => {
    let name =
      "re_jest_matcher_" ++ string_of_int(Counter.next(matcherCounter));
    maybeRegisterMatcher(name, buildMatcher);
    let matcher = (actualThunkThunk, expectedThunkThunk) => {
      let r = Jest.expect(actualThunkThunk());
      let _ = expectReasonRootMatcher(r, (name, expectedThunkThunk()));
      ();
    };
    matcher;
  };
  /* TODO: Can we remove the cast? */
  external cast: 'a => 'b = "%identity";
  let createMatcher = cast(safeCreateMatcher);
};

let createStringUtils = actual => {
  toBeEmpty: () => {
    let r = Jest.expect(actual);
    let _ = Jest.toEqual(r, "");
    ();
  },
  toEqual: expected => {
    let r = Jest.expect(actual);
    let _ = Jest.toEqual(r, expected);
    ();
  },
  toMatchSnapshot: () => {
    let r = Jest.expect(actual);
    let _ = Jest.toMatchSnapshot(r);
    ();
  },
  not: {
    toBeEmpty: () => {
      let r = Jest.expect(actual);
      let _ = Jest.notToEqual(r, "");
      ();
    },
    toEqual: expected => {
      let r = Jest.expect(actual);
      let _ = Jest.notToEqual(r, expected);
      ();
    },
  },
};

let createBoolUtils = actual => {
  toBe: expected => {
    let expected = expected;
    let actual = actual;
    let r = Jest.expect(actual);
    let _ = Jest.toBe(r, expected);
    ();
  },
  toBeTrue: () => {
    let expected = true;
    let actual = actual;
    let r = Jest.expect(actual);
    let _ = Jest.toBe(r, expected);
    ();
  },
  toBeFalse: () => {
    let expected = false;
    let actual = actual;
    let r = Jest.expect(actual);
    let _ = Jest.toBe(r, expected);
    ();
  },
  not: {
    toBe: expected => {
      let expected = expected;
      let actual = actual;
      let r = Jest.expect(actual);
      let _ = Jest.notToBe(r, expected);
      ();
    },
    toBeTrue: () => {
      let expected = true;
      let actual = actual;
      let r = Jest.expect(actual);
      let _ = Jest.notToBe(r, expected);
      ();
    },
    toBeFalse: () => {
      let expected = false;
      let actual = actual;
      let r = Jest.expect(actual);
      let _ = Jest.notToBe(r, expected);
      ();
    },
  },
};

let createIntUtils = (actual: int): intUtilsWithNot => {
  toBe: expected => {
    let r = Jest.expect(actual);
    let _ = Jest.toBe(r, expected);
    ();
  },
  not: {
    toBe: expected => {
      let r = Jest.expect(actual);
      let _ = Jest.notToBe(r, expected);
      ();
    },
  },
};

let createFloatUtils = actual => {
  toBeCloseTo: (~digits=2, expected) => {
    let r = Jest.expect(actual);
    let _ = Jest.toBeCloseTo(r, expected, digits);
    ();
  },
  not: {
    toBeCloseTo: (~digits=2, expected) => {
      let r = Jest.expect(actual);
      let _ = Jest.notToBeCloseTo(r, expected, digits);
      ();
    },
  },
};

module Equality = {
  let string = (==);
  let bool = (===);
  let int = (===);
  let float = (a, b) => Caml.Pervasives.abs_float(a -. b) < 1e-2;
};

/* TODO: Write this in JS and use a custom jest matcher for nicer output. */
let rec listEquals =
        (listA: list('a), listB: list('a), equals: ('a, 'a) => bool): bool =>
  switch (listA, listB) {
  | ([], []) => true
  | ([], _) => false
  | (_, []) => false
  | ([a, ...listAExtra], [b, ...listBExtra]) when equals(a, b) =>
    listEquals(listAExtra, listBExtra, equals)
  | _ => false
  };

let areListsEqual: type a. (collectionKind(a), list(a), list(a)) => bool =
  (kind, actual, expected) =>
    switch (kind) {
    | String => listEquals(actual, expected, Equality.string)
    | Bool => listEquals(actual, expected, Equality.bool)
    | Int => listEquals(actual, expected, Equality.int)
    | Float => listEquals(actual, expected, Equality.float)
    | Custom(equals) => listEquals(actual, expected, equals)
    };

let createListOfUtils:
  type a. (collectionKind(a), list(a)) => listOfUtilsWithNot(a) =
  (kind, actual) => {
    toEqual: expected => {
      let equal = areListsEqual(kind, actual, expected);
      let r = Jest.expect(equal);
      let _ = Jest.toBe(r, true);
      ();
    },
    not: {
      toEqual: expected => {
        let equal = areListsEqual(kind, actual, expected);
        let r = Jest.expect(equal);
        let _ = Jest.toBe(r, false);
        ();
      },
    },
  };

/* TODO: Write this in JS and use a custom jest matcher for nicer output. */
let arrayEquals =
    (arrayA: array('a), arrayB: array('a), equals: ('a, 'a) => bool): bool => {
  let n1 = Array.length(arrayA);
  let n2 = Array.length(arrayB);
  if (n1 !== n2) {
    false;
  } else {
    let equal = ref(true);
    let i = ref(0);
    while (equal^ && i^ < n1) {
      if (!equals(arrayA[i^], arrayB[i^])) {
        equal := false;
      };
      incr(i);
    };
    equal^;
  };
};

let areArraysEqual: type a. (collectionKind(a), array(a), array(a)) => bool =
  (kind, actual, expected) =>
    switch (kind) {
    | String => arrayEquals(actual, expected, Equality.string)
    | Bool => arrayEquals(actual, expected, Equality.bool)
    | Int => arrayEquals(actual, expected, Equality.int)
    | Float => arrayEquals(actual, expected, Equality.float)
    | Custom(equals) => arrayEquals(actual, expected, equals)
    };

let createArrayOfUtils:
  type a. (collectionKind(a), array(a)) => arrayOfUtilsWithNot(a) =
  (kind, actual) => {
    toEqual: expected => {
      let equal = areArraysEqual(kind, actual, expected);
      let r = Jest.expect(equal);
      let _ = Jest.toBe(r, true);
      ();
    },
    not: {
      toEqual: expected => {
        let equal = areArraysEqual(kind, actual, expected);
        let r = Jest.expect(equal);
        let _ = Jest.toBe(r, false);
        ();
      },
    },
  };

let createExpectUtils = ext => {
  string: createStringUtils,
  bool: createBoolUtils,
  int: createIntUtils,
  float: createFloatUtils,
  listOf: createListOfUtils,
  arrayOf: createArrayOfUtils,
  ext,
};

let createTestUtils = ext => {
  let expectUtils = createExpectUtils(ext);
  {
    expect: expectUtils,
    expectTrue: actual => expectUtils.bool(actual).toBeTrue(),
    expectFalse: actual => expectUtils.bool(actual).toBeFalse(),
    expectEqual: (actual, expected) =>
      expectUtils.bool(actual == expected).toBeTrue(),
    expectNotEqual: (actual, expected) =>
      expectUtils.bool(actual != expected).toBeTrue(),
    expectSame: (actual, expected) =>
      expectUtils.bool(actual === expected).toBeTrue(),
    expectNotSame: (actual, expected) =>
      expectUtils.bool(actual !== expected).toBeTrue(),
  };
};

let beforeEach = fn => {
  let jsFn = () => {
    fn();
    undefined;
  };
  let _ = Jest.beforeEach(jsFn);
  ();
};

let beforeAll = fn => {
  let jsFn = () => {
    fn();
    undefined;
  };
  Jest.beforeAll(jsFn)->ignore;
};

let createTest = (test, ext, name, fn) => {
  let testUtils = createTestUtils(ext);
  let jsFn = () => {
    fn(testUtils);
    undefined;
  };
  let _ = test(name, jsFn);
  ();
};

let createDescribeUtils = ext => {
  beforeEach,
  beforeAll,
  test: createTest(Jest.test, ext),
  testOnly: createTest(Jest.testOnly, ext),
  testSkip: createTest(Jest.testSkip, ext),
};

let extendUtils = {createMatcher: Extensions.createMatcher};

let extendDescribe = extFn => {
  {
    describe: (name, fn) => {
      let ext = extFn(extendUtils);
      let describeUtils = createDescribeUtils(ext);
      let jsFn = () => {
        fn(describeUtils);
        undefined;
      };
      let _ = Jest.describe(name, jsFn);
      ();
    },
  };
};

let describe = extendDescribe(_ => ()).describe;

let resetModules = () => {
  let _ = Jest.resetModules();
  ();
};
