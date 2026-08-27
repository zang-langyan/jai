#import "Basic";

LinkedList :: struct {
    data: s64; 
    next:  *LinkedList;
}


fibonacci :: (n : s64) -> s64 {
    a := 1;
    b := 1;
    i := 3;
    while i <= n {
        c := a + b;
        a = b;
        b = c;
        i = i + 1;
    }
    return b;
}

main :: () {
    lst := New(LinkedList); // lst is of type *LinkedList
    lst.data = 0.1;

    a : *LinkedList =  New(LinkedList);  // a is of type *LinkedList
    a.data = 12;
    lst.next = a;
    
    b  := New(LinkedList);
    b.data = 24;
    a.next = b;
    
    c  := New(LinkedList);
    c.data = 36;
    b.next = c;

    c.next = null; // the list ends here for the time being

    print("Our list is: %p\n", lst); // => Our list is: 149_5343_0090
    print("Our << list is: %f\n", << lst); // => Our list is: {0, 149_5343_0090}
    print("List data is %f -> %d -> %d -> %d \n", lst.data, a.data, b.data, c.data); 
    print("List is %p, %p, %p, %p \n", lst, a, b, c); 
    print("List is %p -> %p -> %p -> %p \n", lst.next, a.next, b.next, c.next); 
    // => List data is 0 -> 12 -> 24 -> 36

    print("fibonacci(10) = %d\n", fibonacci(10));
    // free memory:
    free(a); free(b); free(c); free(lst);
}