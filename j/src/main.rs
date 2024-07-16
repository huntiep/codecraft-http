use std::net::TcpStream;
use std::io::{Read, Write};

fn main() {
    let mut v = Vec::with_capacity(32);
    for _ in 0..32 {
        let c = TcpStream::connect("127.0.0.1:4221").unwrap();
        v.push(c);
    }
    let msg = "GET / HTTP/1.1\r\n\r\n";
    for ch in msg.chars() {
        println!("{}", ch);
        for c in &mut v {
            c.write(&[ch as u8]).unwrap();
        }
        std::thread::sleep_ms(1000);
    }
    let mut buf = vec![0; 1024];
    for mut c in v {
        c.read(&mut buf);
        println!("{}", String::from_utf8(buf.clone()).unwrap());
    }
}
