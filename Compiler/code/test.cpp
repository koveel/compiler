make :: [T, Args..] = (args: Args..)
	return T(args..);
    
//c2 :: typeof(constant0)&; // u64&
c0 :: i32(a, b: i32) return a + b;
c1 :: [T] = T(a, b: T) return a + b;
//    
//c1 :: 10; // u64
//c5 :: ("hello"); // string literal
//c2 :: (); // parameterless void function type