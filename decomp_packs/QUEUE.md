# Packs.dll queue

Lanes are disjoint by file. Sizes are real extents (bytes). "fp" = x87 instruction count.

## Status after round H11 (34 of 51 matched)

Matched: PackFile ctor/dtor/Tell/Seek/Open/Read/Write, BMPLoad, BMPSave, ExecUnpack, PutZeroBit, PutOneBit, EncodeStart,
PackBlocks, PackLevels, EmitBit, EncodeBit, ConvertBgrToPlanes, EncodeBandPlanes, DecodeSignedValue, DecodeJpegRgb, GetBit,
AdvanceOverToken, StoreCoefficient; batch 399: GetBits (unsigned short accumulator), PackPlanesToWords (carry bits kept
in unsigned short locals), ReadStreamHeader (declaration position of `consumed`); batch 400: PutBits (the original
inlines PutZeroBit but calls PutOneBit), UnpackRecords (no byte-typed `moreChunks` local); batch 401: MMV3 (block-scoped blockOffset), DequantizeBand
(scaledLevel hoisted), DecodeChunk (`while (true)` outer loop: see NOTES "Inline sites"); batch 402: BMPPack
(LevelModelSet is one 288-byte array: the /GS cookie; one-pass attempt loop kept by the original).

Open, closest first (diffs from build output; all register/slot residues unless noted):
- 10005a40 EncodeJpegRgb (40: ours biases srcRow by +2; rowBuffer/stride swap ebx/ebp), 10005470 SaveJpeg (130, same loop)
- 100060a0 ExtractBaseName (138: zero register reused for the memset stores), 10003490 ToRgbBytes (231)
- 10003bb0 QuantiseBands (96 lines: original frame 0x30 vs 0x2c, band offsets spilled to dead argument slots)
- 10004540 EncodeBand (180 lines: consumed/position on the stack, count in edx), 100019d0 QuantiseBand (frame 0x4c vs 0x48)
- 100015c0 WriteStream (register rotation), 10004e50 EncodeStream (119 instr; band[1] is the induction pointer)
- 10001000 BandStatistics (1488 vs 1470: ebx scratch then re-zeroed; previous in edi)
- 10002180 TransformLevel: transcribed in the original's shape (23 named window locals, 12 column pointers), behaviour
  checked against the loop version on 400 random planes; 2576 vs ~2976 bytes, register/slot allocation differs
- 10003e60 DecodeBand: MATCH (round H11, see below)
- 10006e90 ExecPack (403: reload block before the loop top, two epilogues), 10006a40 EncodeChunk (510: the original
  also expands PutOneBit once at the match flag; run length in eax)
- 10004a70 BMPUnpack (761: original caches the GetTickCount import in edi, data in ebp)
- 10004120 DequantizeLevels, 10004830 DecodeLevels (original frames larger, hoisted address temps)
- 10002d20 InverseWaveletLevel: transcribed as two hand-expanded passes (checked on 60000 random planes); 1296 vs 1338
  bytes, frame 0x34 vs 0x40
- 10006a40 EncodeChunk: all but the candidate-search loop identical once PutOneBit expands at the match flag (only an
  open-coded copy does that honestly-looking; not adopted); 10003bb0 QuantiseBands / 10004e50 EncodeStream: the original
  keeps three hoisted pointer differences, ours folds right and diagonal

Notes: encoder globals (0x1003afe0..) and decoder globals (0x1003b008..) are separate variables; the scan tables
`g_scanColumn/g_scanRow` (0x10038040/440, packer) and `g_blockScanX/Y` (0x10038848/c48, unpacker) are separate too.
Globals are extern only (relocations are masked). Helpers need external linkage for compare_obj to see them.
Declaration position of a later-initialised local changes register assignment; try it before expression rewrites.
A permuter adapter for this tree: compile.cmd/score.py pointed at decomp_packs (see ../tools/gurumin_permuter).

Round H9 (duplicated tails / in-place reads / inverted tests applied to the open functions; none newly matched):
- ToRgbBytes: the original's three low-clamp stubs (`mov reg,0x10000; jmp tail`) sit after the ret and the high-clamp
  path falls into one shared store tail. Tried: `<=` first with else-if, nested ternary, early returns, a store helper
  that puts `out[(*pos)++]` in each branch: the layout stays "else arm inline after a jmp", so the stub placement is not
  reachable by arm order. Register residue as before (outPosition ebx vs esi).
- EncodeJpegRgb: the original copies from[0], from[1], from[2] without a bias; ours biases srcRow by +2 (ecx-2,-1,0)
  whatever the inline loop spelling (for/while/countdown/indexed/end pointer, const source, store order of srcRow/row).
- DecodeBand: in-place dx/dy reads (133 diffs), local for by, unsigned i, do-while: all worse or equal; the scan index
  stays in edx (original ecx) and blockY/i swap slots.
- ExecPack: loop-exit return written inside the loop (`if (chunksLeft == 0) return packedSize;` with `while (true)`) and
  150 random orders of the stream setup statements do not change the first difference: the original stores
  `stream.reserved = 8` as an immediate after `mov eax,8` (packedSize) while ours shares eax.
- EncodeStream: one induction `band` with band+1/band+2 shortens the code (352 -> 320, original 345) but diverges more;
  original keeps two hoisted band differences plus the width/height difference, with an 8 byte frame.
- DecodeLevels: reordering the three offset/width/height statements changes nothing.
- Method note: store/statement order was searched with an instruction-level metric (sbs `*` lines); only the sound3d
  key/header functions respond to it.

Round H11 (named-local-vs-temporary rule, early-return tails, parameter reuse):
- DecodeBand MATCH (34 of 51). Two levers: (1) `dy` is not a named local: `blockY + g_blockScanY[i]` is written in the
  bounds test and in the store index (a named dy was the "i/blockY slot" swap); (2) the clamp tail of the inlined
  ProbabilityFromScale is `if (probability > ARITH_PROB_MAX) return ARITH_PROB_MAX; return probability;` (early return,
  not an assignment). With the assignment form the compiler reloads the scan offset `i` after the clamp; with the early
  return it reloads it right after the division, as the original does. Also `magnitude` is its own local instead of
  negating `coefficient` in place (neutral for the bytes, no reused meaning). Same trick did nothing for ToRgbBytes
  (SampleToByte: else-arm early return folds 0x10000>>7 to a constant and moves further away), BiasByte,
  RoundingOffset or QuantiseCoefficient.
- QuantiseBand: `scale *= QUANT_DIVISOR_PER_SCALE` reuses the parameter (the original keeps the divisor in the dead
  `scale` argument slot, [esp+0x7c]); 1248 -> 929 differing bytes. Still open: the original frame is 0x4c (ours 0x48),
  the error sums sit at 0x10..0x18 and the counts at 0x1c..0x24 (ours the other way round), 1954 vs 1904 bytes.
- No effect: ToRgbBytes without the named sample/plane locals, EncodeJpegRgb/SaveJpeg with `row` in the loop, inline
  stride, `src` advanced directly (the +2 bias comes with srcRow being merged with the inner loop pointer);
  BandStatistics second slope as its own local (worse); DequantizeLevels record lookup by `levels - level`;
  QuantiseBands without `right`, with scales built through a `half` local.
