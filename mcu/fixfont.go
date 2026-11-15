package main

import (
	"fmt"
	"io"
	"log"
	"os"
)

func main() {
	f, err := os.Open("TRIVGA8.F16")
	if err != nil {
		log.Fatal("Failed opening file: " + err.Error())
	}

	outf, err := os.Create("trivga.inc")
	if err != nil {
		log.Fatal("Failed opening file: " + err.Error())
	}
	fmt.Fprintf(outf, "data:\n")

	buf := make([]byte, 1)

	for lineIdx := 0; lineIdx < 16; lineIdx++ {
		f.Seek(int64(lineIdx), 0)
		fmt.Fprintf(outf, "    .db ")
		for charIdx := 0; charIdx < 256; charIdx++ {
			n, _ := io.ReadFull(f, buf)
			f.Seek(15, 1)
			if n != 1 {
				log.Fatal("Failed reading byte")
			}
			if charIdx > 0 {
				fmt.Fprintf(outf, ", 0x%02x", buf[0])
			} else {
				fmt.Fprintf(outf, "0x%02x", buf[0])
			}
		}
		fmt.Fprint(outf, "\n")
	}

	f.Close()
	outf.Close()
}
