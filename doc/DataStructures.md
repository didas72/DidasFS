Data structures and layout in drive.


## Defined values
- Block size (BS) = 32k (32768)


## Global structure
Offset | Length | Type | Purpose
-- | -- | -- | --
0 | 4 | int | Magic number = 0x69DEAD69
4 | 4 | int | Block map size (BMP) = block count / 8
8 | 8 | N/A | Reserved
16 | BMP | N/A | Block map
16 + BMP | BMP*BS | N/A | Data blocks

**Remark:** Header size = 16


## Block structure
Offset | Length | Type | Purpose
-- | -- | -- | --
0 | 8 | int | Previous block index
8 | 8 | int | Next block index
16 | 4 | int | Used block space
20 | 4 | N/A | Reserved (zero out)
24 | 8 | N/A | Reserved (zero out)
32 | 32736 | N/A | Data

**Remark:** Header size = 32  
**Remark:** Files per directory block = 682


## Entry pointer (structures inside dirs)
Offset | Length | Type | Purpose
-- | -- | -- | --
0 | 8 | int | First block (0 is invalid)
8 | 8 | int | Last block (0 is invalid)
16 | 2 | bits | Flags
18 | 2 | N/A | Reserved
20 | 28 | char[] | Name (pad ending with zeros)

**Remark:** Total size = 48
**Remark:** Name length = 28

### Entry pointer flags
Bit | Name | Meaning
-- | -- | --
0 | Dir | 0=File; 1=Directory
1-15 | Res | Reserved, zero out

