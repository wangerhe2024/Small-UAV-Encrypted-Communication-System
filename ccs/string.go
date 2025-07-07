package main

import (
    "math/rand"
    "strings"
    "time"
)

const charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

func randomString(n int) string {
    sb := strings.Builder{}
    sb.Grow(n)
    for i := 0; i < n; i++ {
        sb.WriteByte(charset[rand.Intn(len(charset))])
    }
    return sb.String()
}

func GenerateRandomString(n int) string {
    rand.Seed(time.Now().UnixNano())
	return randomString(n)
}

