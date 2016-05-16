let nx = 200
let ny = 100

printfn "P3"
printfn "%d %d" nx ny
printfn "255"
for j in [ny-1 .. -1 .. 0] do
    for i in [0 .. nx-1] do
        let r = float i / float nx
        let g = float j / float ny
        let b = 0.2
        let ir = int (255.99 * r)
        let ig = int (255.99 * g)
        let ib = int (255.99 * b)
        printfn "%d %d %d" ir ig ib
