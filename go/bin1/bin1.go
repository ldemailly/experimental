package main
import (
    "fmt"
    "runtime"
)

func main() {
    _, filename, _, ok := runtime.Caller(0)
    fmt.Printf("ok=%v, fname=%s\n", ok, filename)
}
