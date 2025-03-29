fn factoriel (n : u64)-> u64{
    if n==0{
        1
    } else {
        n * factoriel(n-1)
    }
}

fn main(){
    println!("{}",factoriel(10));
}