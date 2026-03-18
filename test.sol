// comment! /*
/* this is a block
 * comment!
 /*
  Nesting oo
  */
 * */
import basic

const the_constant: int = 5;

some_function: (p: int)

enum Value {
	FIRST,
	SECOND,
	THIRD,
}

main: (args: []string) {
	x: int = -1
	y: uint = 1

	x2: i64
	x3: u64

	val: Value = .FIRST print(val)

	c: char = 'a'

	arr: []int = {1, 2, 3}

	for x in arr {
		print(x)
	}

	// string interpolation
	x4: int = 100
	s: string = $"{x4 + the_constant}" // 105

	print($"One plus two is: {1+2}")

	// dynamic arrays
	arr2: [..]int = {}
	arr2.push(5)

	// pointers and local scope functions
	test: (p: *int) {
		print($"address: {p}, value: {*p}")
	}
	value := 5
	test(value)
}

/////////////////////////////////////////////////////////////////////////////////////////////

// macro system ideas:
// (inline expression/block replacement)
$my_macro: (x) {
	// u can do anything here.
	print("MACRO!")
	return x+5; // the "final expression", which can be assigned as a value
}

main: () {
	y := my_macro(5)
}

// transformed to ->
main: () {
	print("MACRO!")
	y := 5+5
}

// generics / polymorphism:
foo: (a, b: $T, $Key) -> $T {
	print(Key)
	return x+y
}
another: (x: [$N]int) // accepts any size static array