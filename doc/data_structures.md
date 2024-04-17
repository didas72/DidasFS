# Data structures

Data structures and layout in drive.

## Defined values

- Block size (BS) = 32k (32768)

## Global structure

Offset | Length | Type | Purpose
-- | -- | -- | --
0 | 4 | int | Magic number = 0x69DEAD69
4 | 4 | int | Block map size (BMS) = block count / 8
8 | 8 | N/A | Reserved
16 | BMS | N/A | Block map
DTS - 32 | 32 | entry_pointer | Root pointer
DTS\* | BMS\*8\*BS | N/A | Data blocks

**Remark:** Header size = 16  
**Remark:** DTS (DaTa Start) = 512 \* ceil((16 + BMS - 32) / 512)  
**Remark:** Maximum partition capacity = 1.407\*10^14 B ~ 127.937 TB

## Block structure

Offset | Length | Type | Purpose
-- | -- | -- | --
0 | 4 | int | Previous block index
4 | 4 | int | Next block index
8 | 4 | int | Used block space
12 | 4 | N/A | Reserved (zero out)
16 | 32752 | N/A | Data

**Remark:** Header size = 16  
**Remark:** Files per directory block = 1023(.5)

## Entry pointer (structures inside dirs)

Offset | Length | Type | Purpose
-- | -- | -- | --
0 | 4 | int | First block index (0 is invalid)
4 | 4 | int | Last block index (0 is invalid)
8 | 2 | bits | Flags
10 | 2 | N/A | Reserved (zero out)
12 | 20 | char[] | Name (pad ending with zeros)

**Remark:** Total size = 32  
**Remark:** Name length = 20

### Entry pointer flags

Bit | Name | Meaning
-- | -- | --
0 | Dir | 0=File; 1=Directory
1-15 | Res | Reserved, zero out
