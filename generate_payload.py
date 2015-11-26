from ropgadget.args import Args as ROPGadgetArgs
from ropgadget.core import Core as ROPGadgetCore
from capstone import *
from capstone.arm import *
import argparse
import random
import struct

# TODO: Add support for JOP
# TODO: Add support for making function calls

MAX_SCRATCH_SIZE = 1024
BLACKLIST_OPS = (
  ARM_INS_PUSH, ARM_INS_POP, ARM_INS_LDMDA, ARM_INS_LDMDB, 
  ARM_INS_LDM, ARM_INS_STMDA, ARM_INS_STMDB, ARM_INS_STM, 
  ARM_INS_STMIB, ARM_INS_BL, ARM_INS_BLX, ARM_INS_BX, 
  ARM_INS_BXJ, ARM_INS_B, ARM_INS_CBZ, ARM_INS_CBNZ, 
  ARM_INS_BKPT, ARM_INS_SVC, ARM_INS_SMC, ARM_INS_MRC, 
  ARM_INS_MRC2, ARM_INS_MRRC, ARM_INS_MRRC2, ARM_INS_MRS, 
  ARM_INS_MSR, ARM_INS_MCR, ARM_INS_MCR2, ARM_INS_MCRR, 
  ARM_INS_MCRR2
)

class ROPGenerate:
  def __init__(self, ropgadget):
    # find gadgets
    ropgadget.do_load(None, True)
    arm_gadgets = ropgadget.gadgets()
    ropgadget.do_thumb("enable", True)
    ropgadget.do_load(None, True)
    thumb_gadgets = ropgadget.gadgets()

    # set up capstone
    self.__md = Cs(CS_ARCH_ARM, CS_MODE_ARM)
    self.__md.detail = True
    # hack to get needed data type
    self.__cs = self.__md.disasm(bytes([0x00, 0x00, 0xA0, 0xE1]), 0).next()

    # find pop gadgets
    self.__pop_gadgets = self.__get_pop_gadgets(arm_gadgets, False)
    self.__pop_gadgets += self.__get_pop_gadgets(thumb_gadgets, True)

  def __get_pop_gadgets(self, gadgets, thumb):
    self.__md.mode = CS_MODE_THUMB if thumb else CS_MODE_ARM
    pop_gadgets = []
    seen = set()
    for gadget in gadgets:
      mygadget = gadget
      mygadget['mem_base'] = []
      mygadget['regs'] = []
      mygadget['gadget'] = ''
      if thumb:
        mygadget['vaddr'] = mygadget['vaddr'] | 1;
      first = True
      for insn in self.__md.disasm(gadget['bytes'], gadget['vaddr']):
        mem_base_reg = ARM_REG_INVALID
        mem_imm = 0
        regs = []
        touch_sp_pc = False
        invalid = False
        mygadget['gadget'] += '%s %s ; ' % (insn.mnemonic, insn.op_str) 
        for i in insn.operands:
          if i.type == ARM_OP_MEM:
            if not first:
              invalid = True
              break # no support data instruction after reg instruction
            if i.mem.index != 0 or i.mem.scale != 1 or i.mem.disp != 0:
              invalid = True
              break # no support for these types
            mem_base_reg = i.mem.base
          elif i.type == ARM_OP_IMM:
            mem_imm = i.imm
          elif i.type == ARM_OP_REG:
            if i.reg == ARM_REG_SP or i.reg == ARM_REG_PC:
              touch_sp_pc = True
            regs += [i.reg]
          else:
            invalid = True
            break # no support for these operands
        first = False
        if invalid:
          break
        if mem_base_reg != ARM_REG_INVALID and mem_imm < MAX_SCRATCH_SIZE:
          mygadget['mem_base'].append(mem_base_reg)
        if insn.id == ARM_INS_POP and regs[-1] == ARM_REG_PC and insn.cc in [ARM_CC_AL, ARM_CC_INVALID] and ARM_REG_SP not in regs:
          mygadget['regs'] = regs
          break
        elif touch_sp_pc or insn.id in BLACKLIST_OPS:
          break
      mygadget['gadget'] = mygadget['gadget'][:-2]
      if len(mygadget['regs']) > 0 and mygadget['vaddr'] not in seen:
        pop_gadgets.append(mygadget)
        seen.add(mygadget['vaddr'])
    return pop_gadgets

  def get_chain(self, output, gadget, regs={}):
    output.write(struct.pack('<I', gadget['vaddr']))
    print '0x%08X = %s' % (gadget['vaddr'], gadget['gadget'])
    for i in range(len(gadget['regs'])-1):
      if gadget['regs'][i] in regs:
        data = regs[gadget['regs'][i]]
      else:
        data = random.randint(0, 0xFFFFFFFF)
      output.write(struct.pack('<I', data))
      print '0x%08X = %s' % (data, self.__cs.reg_name(gadget['regs'][i]))
    return len(gadget['regs'])

  def get_random_chain(self, output):
    while True:
      gadget = random.choice(self.__pop_gadgets)
      if len(gadget['mem_base']) > 0:
        continue # pick again, non memory gadget needed
      return self.get_chain(output, gadget)

  def get_random_mem_chain(self, scratch, output):
    gadgets = self.__pop_gadgets
    random.shuffle(gadgets)
    i = 0
    for mem_gadget in gadgets:
      if len(mem_gadget['mem_base']) == 0:
        continue
      need = set(mem_gadget['mem_base'])
      regs = {k: scratch for k in need}
      for reg_gadget in gadgets:
        if len(reg_gadget['mem_base']) > 0:
          continue # don't do it recursively
        have = set(reg_gadget['regs']) & need
        if not have:
          continue
        i += self.get_chain(output, reg_gadget, regs)
        need = need - have
      if len(need) > 0:
        break # not possible
      i += self.get_chain(output, mem_gadget)
      break
    return i

def main():
  parser = argparse.ArgumentParser(description='Generate payload for ropsimulate.')
  parser.add_argument('-s', '--seed', type=str, help='Random seed.')
  parser.add_argument('-e', '--end', type=str, metavar="<hexaddr>", default=0, help='Address of final gadget in payload.')
  parser.add_argument('-n', '--size', type=int, metavar="<number>", default=4096, help='Minimum size of payload in bytes')
  parser.add_argument('-w', '--memweight', type=int, metavar="0-99", default=75, choices=range(100), help='Weight of memory access gadgets (0-99). Default is 75, 0 if scratch buffer not specified.')
  parser.add_argument('-m', '--scratch', type=str, metavar="<hexaddr>", default='0', help='Address of scratch space for memory gadgets. No memory gadgets will be used without this argument.')
  parser.add_argument('input', type=str, help='Input ELF executable')
  parser.add_argument('output', type=argparse.FileType('wb'), help='Output ROP payload')
  args = parser.parse_args()
  random.seed(args.seed)
  ropg = ROPGadgetCore(ROPGadgetArgs(['--console']).getArgs())
  ropg.do_binary(args.input, True)
  i = 0
  scratch = int(args.scratch, 16)
  generate = ROPGenerate(ropg)
  while i < args.size/4:
    if scratch > 0 and random.randrange(100) < args.memweight:
      i += generate.get_random_mem_chain(scratch, args.output)
    else:
      i += generate.get_random_chain(args.output)
  final = int(args.end, 16)
  if (final > 0):
    args.output.write(struct.pack('<I', final))
    i += 1
    print '0x%08X = end' % final
  print 'Total bytes: %d' % (i*4)

if __name__ == "__main__":
  main()
