[BotW_Find3DFrameBuffer_V208]
moduleMatches = 0x6267BFD0

.origin = codecave


magicClearValue:
.float 0.5
.float 0.123456789
.float 0.987654321
.float 1.0


; r10 holds the agl::RenderBuffer object
hookPreHDRComposedImage:
mflr r6
stwu r1, -0x20(r1)
stw r6, 0x24(r1)

stfs f1, 0x20(r1)
stfs f2, 0x1C(r1)
stfs f3, 0x18(r1)
stfs f4, 0x14(r1)
stw r3, 0x10(r1)

lis r3, magicClearValue@ha
lfs f1, magicClearValue@l+0x0(r3)
lfs f2, magicClearValue@l+0x4(r3)
lfs f3, magicClearValue@l+0x8(r3)
lfs f4, magicClearValue@l+0xC(r3)

mr r3, r10
addi r3, r3, 0x1C ; r3 is now the agl::RenderBuffer::mColorBuffer array
lwz r3, 0(r3) ; r3 is now the agl::RenderBuffer::mColorBuffer[0] object
cmpwi r3, 0
beq skipClearingColorBuffer
addi r3, r3, 0xBC ; r3 is now the agl::RenderBuffer::mColorBuffer[0]::mGX2FrameBuffer object

bl import.gx2.GX2ClearColor

skipClearingColorBuffer:
lfs f1, 0x20(r1)
lfs f2, 0x1C(r1)
lfs f3, 0x18(r1)
lfs f4, 0x14(r1)
lwz r3, 0x10(r1)

lwz r6, 0x24(r1)
mtlr r6
addi r1, r1, 0x20

lwz r6, 0x238(r11) ; original instruction
blr

0x039DA8D8 = bla hookPreHDRComposedImage