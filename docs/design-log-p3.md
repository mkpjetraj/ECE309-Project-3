# Project 3 Design Log
**Author:** Kole Pjetraj · **P2 code used:** my own!
**Tools used:** I used Claude to help with outlining the ideal workflow, understanding the existing codebase / how the files communicate, and organizingthe bigger-picture logic with the tool registry. 

## 1. Hash table
I used chaining, it is simpler to understand and implement correctly for me. find only has to do a short chain walk. chains are std::vector. I rejected open addressing because with a full table, find for a missiing name loops forever and has to resize early

the hash function is FNV-1a 64-bit: XOR char in, multiply by FNV prime for every char. Order matters. returns full width; index_for reduces it. Load factor threshhold 1.0, sade for chaining as a chain can always grow. Resize policy is to double the bucket count, stays a power of 2 and amortized O(1) additions.

My Hash: FNV-1a 	
Bucket Count: 16384	
Load Factor: 0.6104	(last resize is at tool 8193)(10000/16384)
longest chain: 6

Alt hash: s[0] + s.size()	
bucket count: 16384		
longest chain: 9000 (only 4 buckets used)

weak hash bad because every name starts with t, only 4 lengths means 4 possible values. 

add() is amortized because resize to 2n costs O(n) but ~n (cheap) adds come first - it ends up being constant per add. 

## 2. Pointer stability
Buckets hold unique_ptr<Tool> which is an address, the tool lives om the heap then and never moves. Rehashing moves the unique_ptr, not the tool. Storing tools by value would move them on rehash and leave pointers dangling. 

## 3. Calculator trace
the nums stack holds numbers waitiing, and ops holds operators waiting plus left parentheses (

trace 2 * (3 + 4) - 5: 

| Token | nums | ops | why |
|---|---|---|---|
| 2 | 2 | | number, push |
| * | 2 | * | ops empty, push, nothing to work on yet |
| ( | 2 | * ( | push, put a wall on * |
| 3 | 2 3 | * ( | number, push |
| + | 2 3 | * ( + | top is (, push, ( blocks the * |
| 4 | 2 3 4 | * ( + | number, push |
| ) | 2 7 | * | wall is done, doOp (3+4), pop wall ( |
| - | 14 | - | operator * first, doOp (2×7), push - |
| 5 | 14 5 | - | number, push |
| end | 9 | | do everything left, doOp (14−5) |

I tried to write this out as a table, I hope the formatting is preserved.

## 4. Errors as values
these return errors instead of throwingbecause tool failure is somewhat common/normal, and you dont want to crash the harness. If bad input is given that shouldlnt make the whole thing crash. 
Throwing an exception is the right choice for bugs in the actual program, not for bad input. The model will read the error and try something else now. 

## 5. What I would design differently
If I had more time I might implement more specific errors for the calculator instead of just "bad expression", I think that would be cool and would help the user experience.