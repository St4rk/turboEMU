#include "HuC6280.h"


HuC6280::HuC6280() {


}


HuC6280::~HuC6280() {

}


void HuC6280::resetCPU() {
	// Clear the general purpose registers and a
	x    = 0;
	y    = 0;
	a    = 0;

	// On HuC6280 always Interrupt and SoftInterrupt(BRK) is set
	// Decimal and unused 0
	// N,Z,C,V has random values, in this case, 0
	flag = 0;
	flag = SET_FLAG(flag, FLAG_INT);
	flag = SET_FLAG(flag, FLAG_BRK);

	// Total cycles
	t_cycles = 0;

	// Start of Stack
	sp   = 0x1FF;
	// Reset Vector
	pc   = ((memory.readMemory(0xFFFE) << 0x8) | (memory.readMemory(0xFFFF)));

	// Clear MPR Registers
	memory.clearMPR();
}

void HuC6280::push(unsigned char data) {
	memory.writeMemory(sp, data);
	sp = ((sp - 1) & 0x1FF);
}

unsigned char HuC6280::pop() {
	sp = ((sp + 1) & 0x1FF);
	return (memory.readMemory(sp));
}


// These are the addressing mode of HuC6208
// There are Immediate, Relative, 
// Indexed(where reg x and y are used to calculate the addr of operand)
// and non-indexed(in this case, only pc is used)


void HuC6280::immediate() {
	addrReg = pc++;
}

void HuC6280::zeroPage() {
	addrReg = memory.readMemory(pc++);
}

void HuC6280::zeroPage_X() {
	addrReg = memory.readMemory(pc++);
	addrReg = ((x + addrReg) & 0xFF);
}

void HuC6280::zeroPage_Y() {
	addrReg = memory.readMemory(pc++);
	addrReg = ((y + addrReg) & 0xFF);
}

void HuC6280::indirect() {
	addrReg = ((memory.readMemory(pc)) | (memory.readMemory(pc+1) << 8));
	addrReg = ((memory.readMemory(addrReg)) | ((memory.readMemory((addrReg + 1) & 0xFF) << 8)));
	pc +=2;
}

void HuC6280::indexedIndirect_X() {
	addrReg = (memory.readMemory(pc++) + x);
	addrReg = ((memory.readMemory(addrReg)) | ((memory.readMemory((addrReg + 1) & 0xFF) << 8)));

}

void HuC6280::indirectIndexed_Y() {
	addrReg = memory.readMemory(pc++);
	addrReg = (memory.readMemory(addrReg) | (memory.readMemory((addrReg + 1) & 0xFF) << 8));
	addrReg += y;
}

void HuC6280::absolute() {
	addrReg = (memory.readMemory(pc) | (memory.readMemory(pc+1) << 8));
	pc += 2;	
}

void HuC6280::absolute_X() {
	addrReg = (memory.readMemory(pc) | (memory.readMemory(pc+1) << 8)) + x;
	pc += 2;	
}

void HuC6280::absolute_Y() {
	addrReg = (memory.readMemory(pc) | (memory.readMemory(pc+1) << 8)) + y;
	pc += 2;	
}

void HuC6280::absoluteIndirect_X() {
	addrReg = (((memory.readMemory(pc)) | (memory.readMemory(pc+1) << 8)) + x);
	addrReg = ((memory.readMemory(addrReg)) | ((memory.readMemory((addrReg + 1) & 0xFF) << 8)));
	pc +=2;
}

void HuC6280::relative() {
	addrReg = memory.readMemory(pc++);

	// In the case be a signed byte, it only can jump a maximum of 127 bytes forward
	if (addrReg & FLAG_SIGN) addrReg -= 255;
}



void HuC6280::adc() {
	addrReg = memory.readMemory(addrReg);
	int temp = a + (addrReg) + (flag & FLAG_CARRY);

	if (addrReg > 0xFF) SET_FLAG(flag, FLAG_CARRY);
	if ((~(a ^ addrReg)) & (a^temp) & 0x80) SET_FLAG(flag, FLAG_OVER); else CLEAR_FLAG(flag, FLAG_OVER);

	a = (temp & 0xFF);

	if (a) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (a & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else CLEAR_FLAG(flag, FLAG_SIGN);

}

void HuC6280::and_() {
	addrReg = memory.readMemory(addrReg);

	a = (a & addrReg);

	if (a) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (a & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else CLEAR_FLAG(flag, FLAG_SIGN);

}

void HuC6280::asl() {
	unsigned char temp = memory.readMemory(addrReg);

	if (temp & 0x80) SET_FLAG(flag, FLAG_CARRY); else CLEAR_FLAG(flag, FLAG_CARRY);

	temp = ((temp << 1) & 0xFF);

	memory.writeMemory(addrReg, temp);

	if (temp) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (temp & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else CLEAR_FLAG(flag, FLAG_SIGN);
}

void HuC6280::asl_a() {

	if (a & 0x80) SET_FLAG(flag, FLAG_CARRY); else CLEAR_FLAG(flag, FLAG_CARRY);

	a = ((a << 1) & 0xFF);

	if (a) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (a & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else CLEAR_FLAG(flag, FLAG_SIGN);
}

// This opcode use two addressing mode
// the first one is a zeroPage, the second is the relative
// we will uses the addrReg_2 to it
void HuC6280::bbri(char i) {
	addrReg_2 = memory.readMemory(pc++);

	// In the case be a signed byte, it only can jump a maximum of 127 bytes forward
	if (addrReg_2 & FLAG_SIGN) addrReg -= 255;
	addrReg_2 = memory.readMemory(addrReg_2);
	addrReg = memory.readMemory(addrReg);
	if (!(addrReg & i)) {
		pc += addrReg_2;
	}
}

void HuC6280::bcc() {

	addrReg = memory.readMemory(addrReg);

	if (!(flag & FLAG_CARRY)) {
		pc += addrReg;
	}

}

// the same of BBRI
void HuC6280::bbsi(char i) {
	addrReg_2 = memory.readMemory(pc++);

	// In the case be a signed byte, it only can jump a maximum of 127 bytes forward
	if (addrReg_2 & FLAG_SIGN) addrReg -= 255;
	addrReg_2 = memory.readMemory(addrReg_2);
	addrReg = memory.readMemory(addrReg);

	if (addrReg & i) {
		pc += addrReg;
	}
}

void HuC6280::bcs() {
	addrReg = memory.readMemory(addrReg);

	if (flag & FLAG_CARRY) {
		pc += addrReg;
	}

}

void HuC6280::beq() {
	addrReg = memory.readMemory(addrReg);

	if (flag & FLAG_ZERO) {
		pc += addrReg;
	}
}

void HuC6280::bit() {
	addrReg = memory.readMemory(addrReg);

	if (addrReg & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else CLEAR_FLAG(flag, FLAG_SIGN);
	if (addrReg & FLAG_OVER) SET_FLAG(flag, FLAG_OVER); else CLEAR_FLAG(flag, FLAG_OVER);

	if (a & addrReg) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
}

void HuC6280::bmi() {
	addrReg = memory.readMemory(addrReg);

	if (flag & FLAG_SIGN) {
		pc += addrReg;
	}
}

void HuC6280::bne() {
	addrReg = memory.readMemory(addrReg);

	if (!(flag & FLAG_ZERO)) {
		pc += addrReg;
	}
}

void HuC6280::bpl() {
	addrReg = memory.readMemory(addrReg);

	if (!(flag & FLAG_SIGN)) {
		pc += addrReg;
	}
}

void HuC6280::bra() {
	addrReg = memory.readMemory(addrReg);

	pc += addrReg;
}

void HuC6280::brk() {
	pc++;
	push(pc >> 8);
	push(pc & 0xFF);
	SET_FLAG(flag, FLAG_BRK);
	push(flag);
	CLEAR_FLAG(flag, FLAG_DEC);
	SET_FLAG(flag, FLAG_INT);
	pc = ((memory.readMemory(0xFFF6)) | (memory.readMemory(0xFFF7) << 8));
}

void HuC6280::bsr() {
	addrReg = memory.readMemory(addrReg);
	push(pc >> 8);
	push(pc & 0xFF);
	pc += addrReg;
}

void HuC6280::bvs() {
	addrReg = memory.readMemory(addrReg);

	if (flag & FLAG_OVER) {
		pc += addrReg;
	}		
}

void HuC6280::bvc() {
	addrReg = memory.readMemory(addrReg);

	if (!(flag & FLAG_SIGN)) {
		pc += addrReg;
	}
}

void HuC6280::clc() {
	CLEAR_FLAG(flag, FLAG_CARRY);	
}

void HuC6280::cla() {
	a = 0;	
}

void HuC6280::cld() {
	CLEAR_FLAG(flag, FLAG_DEC);	
}

void HuC6280::cli() {
	CLEAR_FLAG(flag, FLAG_INT);	
}

void HuC6280::clv() {
	CLEAR_FLAG(flag, FLAG_OVER);	
}

void HuC6280::cly() {
	y = 0;	
}

void HuC6280::clx() {
	x = 0;	
}

void HuC6280::cpx() {
	addrReg = (x - memory.readMemory(addrReg));

	if (!(addrReg & 0x8000)) SET_FLAG(flag, FLAG_CARRY); else CLEAR_FLAG(flag, FLAG_CARRY); 

	if (addrReg) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (addrReg & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);
}

// TODO: Look for the number of cycles of these instructions
// on the emu-docs "pce-cpu.html" is unknown the number of cycles
void HuC6280::csh() {
	speedMode = SPEED_MODE_HIGH;
}

void HuC6280::csl() {
	speedMode = SPEED_MODE_LOW;
}

void HuC6280::cmp() {
	addrReg = (a - memory.readMemory(addrReg));

	if (!(addrReg & 0x8000)) SET_FLAG(flag, FLAG_CARRY); else CLEAR_FLAG(flag, FLAG_CARRY);

	if (addrReg) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (addrReg & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else CLEAR_FLAG(flag, FLAG_SIGN);

}

void HuC6280::dex() {
	x = (x - 1) & 0xFF;

	if (x) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (x & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else CLEAR_FLAG(flag, FLAG_SIGN);

}

void HuC6280::dec() {
	unsigned char temp = memory.readMemory(addrReg);

	temp = (temp - 1) & 0xFF;

	if (temp) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (temp & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else CLEAR_FLAG(flag, FLAG_SIGN);

	memory.writeMemory(addrReg, temp);
}


void HuC6280::dec_a() {

	a = (a - 1) & 0xFF;

	if (a) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (a & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else CLEAR_FLAG(flag, FLAG_SIGN);

}


void HuC6280::cpy() {
	addrReg = (y - memory.readMemory(addrReg));

	if (!(addrReg & 0x8000)) SET_FLAG(flag, FLAG_CARRY); else CLEAR_FLAG(flag, FLAG_CARRY); 

	if (addrReg) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (addrReg & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);
	
}

void HuC6280::eor() {

	a = (a ^ memory.readMemory(addrReg)) & 0xFF;

	if (a) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (a & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);

}

void HuC6280::inc() {
	unsigned char temp = memory.readMemory(addrReg);

	temp = (temp + 1) & 0xFF;

	if (temp) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (temp & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);

	memory.writeMemory(addrReg, temp);
}

void HuC6280::inc_a() {

	a = (a + 1) & 0xFF;

	if (a) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (a & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);

}

void HuC6280::inx() {
	x = (x + 1) & 0xFF;
	if (x) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (x & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);

}

void HuC6280::dey() {
	y = (y - 1) & 0xFF;
	if (y) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (y & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);
}

void HuC6280::iny() {
	y = (y + 1) & 0xFF;
	if (y) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (y & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);
}

void HuC6280::jmp() {
	pc = addrReg;
}

void HuC6280::jsr() {
	push(pc >> 8);
	push(pc & 0xFF);
	pc = addrReg;
}

void HuC6280::lda() {
	a = memory.readMemory(addrReg);

	if (a) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (a & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);
}

void HuC6280::ldx() {
	x = memory.readMemory(addrReg);

	if (x) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (x & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);

}

void HuC6280::ldy() {
	y = memory.readMemory(addrReg);

	if (y) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (y & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);

}

void HuC6280::lsr() {
	unsigned char temp = memory.readMemory(addrReg);


	if (temp & 0x1) SET_FLAG(flag, FLAG_CARRY); else CLEAR_FLAG(flag, FLAG_CARRY);

	temp = (temp >> 1) & 0xFF;

	if (temp) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (temp & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);

}

void HuC6280::lsr_a() {
	if (a & 0x1) SET_FLAG(flag, FLAG_CARRY); else CLEAR_FLAG(flag, FLAG_CARRY);

	a = (a >> 1) & 0xFF;

	if (a) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (a & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);

}

void HuC6280::ora() {
	a = ((memory.readMemory(addrReg) | a) & 0xFF);

	if (a) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (a & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);	
}

void HuC6280::nop() {
	// Do Nothing : D
}

void HuC6280::pha() {
	push(a);
}

void HuC6280::php() {
	push(flag);		
}

void HuC6280::phx() {
	push(x);	
}

void HuC6280::phy() {
	push(y);	
}

void HuC6280::pla() {
	a = pop();	
	if (a) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (a & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);	

}

void HuC6280::plp() {
	flag = pop();
}

void HuC6280::plx() {
	x = pop();

	if (x) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (x & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);	
}

void HuC6280::ply() {
	y = pop();
	if (y) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (y & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);	

}

void HuC6280::rol() {
	unsigned char temp = memory.readMemory(addrReg);

	if (flag & FLAG_CARRY) {
		if (temp & 0x80) SET_FLAG(flag, FLAG_CARRY); else CLEAR_FLAG(flag, FLAG_CARRY);

		temp = (temp << 1) | FLAG_CARRY;
	} else {
		if (temp & 0x80) SET_FLAG(flag, FLAG_CARRY); else CLEAR_FLAG(flag, FLAG_CARRY);
	
		temp = (temp << 1);		
	}

	memory.writeMemory(addrReg, temp);

	if (temp) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (temp & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);	
}

void HuC6280::rol_a() {
	if (flag & FLAG_CARRY) {
		if (a & 0x80) SET_FLAG(flag, FLAG_CARRY); else CLEAR_FLAG(flag, FLAG_CARRY);

		a = (a << 1) | FLAG_CARRY;
	} else {
		if (a & 0x80) SET_FLAG(flag, FLAG_CARRY); else CLEAR_FLAG(flag, FLAG_CARRY);
	
		a = (a << 1);		
	}

	if (a) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (a & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);	
}

void HuC6280::rmbi(char i) {
	memory.writeMemory(addrReg, (memory.readMemory(addrReg) &~i));
}

void HuC6280::ror() {
	unsigned char temp = memory.readMemory(addrReg);
	if (flag & FLAG_CARRY) {
		if (temp & 0x1) SET_FLAG(flag, FLAG_CARRY); else CLEAR_FLAG(flag, FLAG_CARRY);
		temp = (temp >> 1) | 0x80;
	} else {
		if (temp & 0x1) SET_FLAG(flag, FLAG_CARRY); else CLEAR_FLAG(flag, FLAG_CARRY);
		temp = (temp >> 1);		
	}

	memory.writeMemory(addrReg, temp);

	if (temp) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (temp & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);	

}

void HuC6280::ror_a() {
	if (flag & FLAG_CARRY) {
		if (a & 0x1) SET_FLAG(flag, FLAG_CARRY); else CLEAR_FLAG(flag, FLAG_CARRY);
		a = (a >> 1) | 0x80;
	} else {
		if (a & 0x1) SET_FLAG(flag, FLAG_CARRY); else CLEAR_FLAG(flag, FLAG_CARRY);
		a = (a >> 1);		
	}

	if (a) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (a & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);	

}

void HuC6280::rti() {
	flag = pop();
	pc = ((pop()) | (pop() << 8));

}

void HuC6280::rts() {
	pc = ((pop()) | (pop() << 8));
	pc++;
}

void HuC6280::sax() {
	unsigned char temp = x;
	x = a;
	a = temp;	
}	

void HuC6280::say() {
	unsigned char temp = y;
	y = a;
	a = temp;
}

void HuC6280::sbc() {
	
	addrReg = memory.readMemory(addrReg);
	unsigned short temp = ((a - addrReg) - (flag & FLAG_CARRY));

	if (((a ^ temp) & 0x80) && ((a ^ addrReg) & 0x80)) SET_FLAG(flag, FLAG_OVER); else CLEAR_FLAG(flag, FLAG_OVER);

	if (temp > 0xFF) SET_FLAG(flag, FLAG_CARRY); else CLEAR_FLAG(flag, FLAG_CARRY);

	a = (temp & 0xFF);

	if (a) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (a & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else CLEAR_FLAG(flag, FLAG_SIGN);


}

void HuC6280::sed() {
	SET_FLAG(flag, FLAG_DEC);
}

void HuC6280::sec() {
	SET_FLAG(flag, FLAG_CARRY);
}

void HuC6280::sei() {
	SET_FLAG(flag, FLAG_INT);
}

// TODO: At least, the T flag(the flag unused on 6502) is called "memory operation flag",
// when this flag is enable, many operations which use the register A now use the memory location
// pointed by the register X, so I need re-write EOR, AND, ADC, OR[...]
// Source: http://emu-docs.org/pc%20Engine/pce_doc/pce_cpu.html
void HuC6280::set() {
	SET_FLAG(flag, FLAG_T);	
}

// this operation sets /CE7, A1, and A0 to logical LOW. 
void HuC6280::st0() {
	memory.writeIO(0xE000, memory.readMemory(addrReg));
}

//this operation sets /CE7 and A0 to logical LOW, while setting A1 to logical HIGH. 
void HuC6280::st1() {
	memory.writeIO(0xE002, memory.readMemory(addrReg));	
}

//this operation sets /CE7 to logical LOW, while setting A0 and A1 to logical HIGH. 
void HuC6280::st2() {
	memory.writeIO(0xE003, memory.readMemory(addrReg));		
}

void HuC6280::smbi(char i) {
	memory.writeMemory(addrReg, (memory.readMemory(addrReg) | i));
}

void HuC6280::sta() {
	memory.writeMemory(addrReg, a);	
}

void HuC6280::stx() {
	memory.writeMemory(addrReg, x);
}

void HuC6280::sty() {
	memory.writeMemory(addrReg, y);
}

void HuC6280::stz() {
	memory.writeMemory(addrReg, 0);	
}

void HuC6280::tai() {
	unsigned short src = (memory.readMemory(pc)   | (memory.readMemory(pc+1) << 8));
	unsigned short dst = (memory.readMemory(pc+2) | (memory.readMemory(pc+3) << 8));
	unsigned short len = (memory.readMemory(pc+4) | (memory.readMemory(pc+5) << 8));

	t_cycles += 17 + (6 * len);
	

	unsigned char alter = 0;

	for (unsigned short i = 0; i < len; i++) {
		memory.writeMemory(dst+i, memory.readMemory(src+alter));
		alter ^= 1;
	}

	pc += 6;
}

void HuC6280::sxy() {
	unsigned char temp = x;
	x = y;
	y = temp;	
}

void HuC6280::tami() {
	addrReg = memory.readMemory(pc++);

	// Verify each bit on the immediate value
	// if the bit is set, set the mpr"i" with accumulator
	if (addrReg & 0x1)  memory.setMPRi(0, a);
	if (addrReg & 0x2)  memory.setMPRi(1, a);
	if (addrReg & 0x4)  memory.setMPRi(2, a);
	if (addrReg & 0x8)  memory.setMPRi(3, a);
	if (addrReg & 0x10) memory.setMPRi(4, a);
	if (addrReg & 0x20) memory.setMPRi(5, a);
	if (addrReg & 0x40) memory.setMPRi(6, a);
	if (addrReg & 0x80) memory.setMPRi(7, a);

}

void HuC6280::tax() {
	x = a;

	if (x) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (x & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);	

}

void HuC6280::tay() {
	y = a;

	if (y) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (y & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);	

}


void HuC6280::tia() {
	unsigned short src = (memory.readMemory(pc)   | (memory.readMemory(pc+1) << 8));
	unsigned short dst = (memory.readMemory(pc+2) | (memory.readMemory(pc+3) << 8));
	unsigned short len = (memory.readMemory(pc+4) | (memory.readMemory(pc+5) << 8));

	t_cycles += 17 + (6 * len);
	

	unsigned char alter = 0;

	for (unsigned short i = 0; i < len; i++) {
		memory.writeMemory(dst+alter, memory.readMemory(src+i));
		alter ^= 1;
	}

	pc += 6;
}


void HuC6280::tdd() {
	unsigned short src = (memory.readMemory(pc)   | (memory.readMemory(pc+1) << 8));
	unsigned short dst = (memory.readMemory(pc+2) | (memory.readMemory(pc+3) << 8));
	unsigned short len = (memory.readMemory(pc+4) | (memory.readMemory(pc+5) << 8));

	t_cycles += 17 + (6 * len);
	


	for (unsigned short i = 0; i < len; i++) {
		memory.writeMemory(dst-i, memory.readMemory(src-i));
	}

	pc += 6;
}


void HuC6280::tin() {
	unsigned short src = (memory.readMemory(pc)   | (memory.readMemory(pc+1) << 8));
	unsigned short dst = (memory.readMemory(pc+2) | (memory.readMemory(pc+3) << 8));
	unsigned short len = (memory.readMemory(pc+4) | (memory.readMemory(pc+5) << 8));

	t_cycles += 17 + (6 * len);

	for (unsigned short i = 0; i < len; i++) {
		memory.writeMemory(dst, memory.readMemory(src+i));
	}

	pc += 6;
}

void HuC6280::tii() {
	unsigned short src = (memory.readMemory(pc)   | (memory.readMemory(pc+1) << 8));
	unsigned short dst = (memory.readMemory(pc+2) | (memory.readMemory(pc+3) << 8));
	unsigned short len = (memory.readMemory(pc+4) | (memory.readMemory(pc+5) << 8));

	t_cycles += 17 + (6 * len);

	for (unsigned short i = 0; i < len; i++) {
		memory.writeMemory(dst+i, memory.readMemory(src+i));
	}

	pc += 6;

}

void HuC6280::tmai() {
	addrReg = memory.readMemory(pc++);
	if (addrReg & 0x1)  a = memory.readMPRi(0);
	if (addrReg & 0x2)  a = memory.readMPRi(1);
	if (addrReg & 0x4)  a = memory.readMPRi(2);
	if (addrReg & 0x8)  a = memory.readMPRi(3);
	if (addrReg & 0x10) a = memory.readMPRi(4);
	if (addrReg & 0x20) a = memory.readMPRi(5);
	if (addrReg & 0x40) a = memory.readMPRi(6);
	if (addrReg & 0x80) a = memory.readMPRi(7);		
}

void HuC6280::trb() {
	unsigned char var = memory.readMemory(addrReg);

	if ((a & var) & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else CLEAR_FLAG(flag, FLAG_SIGN);
	if ((a & var) & FLAG_OVER) SET_FLAG(flag, FLAG_OVER); else CLEAR_FLAG(flag, FLAG_SIGN);

	if ((a & var)) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);

	memory.writeMemory(addrReg, ((~a) & var));	
}

void HuC6280::tsb() {
	unsigned char var = memory.readMemory(addrReg);

	if ((a & var) & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else CLEAR_FLAG(flag, FLAG_SIGN);
	if ((a & var) & FLAG_OVER) SET_FLAG(flag, FLAG_OVER); else CLEAR_FLAG(flag, FLAG_SIGN);

	if ((a & var)) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);

	memory.writeMemory(addrReg, (a | var));
}

void HuC6280::tst() {

	if (addrReg & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else CLEAR_FLAG(flag, FLAG_SIGN);
	if (addrReg & FLAG_OVER) SET_FLAG(flag, FLAG_OVER); else CLEAR_FLAG(flag, FLAG_SIGN);
	if (addrReg & addrReg_2) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);

}

void HuC6280::tsx() {
	x = (sp & 0xFF);
	if (x) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (x & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);	

}

void HuC6280::txa() {
	a = x;
	if (a) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (a & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);	

}

void HuC6280::tya() {
	a = y;
	if (a) CLEAR_FLAG(flag, FLAG_ZERO); else SET_FLAG(flag, FLAG_ZERO);
	if (a & FLAG_SIGN) SET_FLAG(flag, FLAG_SIGN); else SET_FLAG(flag, FLAG_SIGN);	

}


void HuC6280::txs() {
	sp = (0x100 | x);
}



void HuC6280::executeCPU() {
	unsigned char opcode = memory.readMemory(pc++);

	t_cycles = 0;

	while (t_cycles < MAX_TICKET) {
		switch (opcode) {

			// ADC
			case 0x69:
				immediate();
				adc();
				t_cycles += 2;
			break;

			case 0x65:
				zeroPage();
				adc();
				t_cycles += 4;
			break;

			case 0x75:
				zeroPage_X();
				adc();
				t_cycles += 4;
			break;

			case 0x72:
				indirect();
				adc();
				t_cycles += 7;
			break;

			case 0x61:
				indexedIndirect_X();
				adc();
				t_cycles += 7;
			break;

			case 0x71:
				indirectIndexed_Y();
				adc();
				t_cycles += 7;
			break;

			case 0x6D:
				absolute();
				adc();
				t_cycles += 5;
			break;

			case 0x7D:
				absolute_X();
				adc();	
				t_cycles += 5;
			break;

			case 0x79:
				absolute_Y();
				adc();
				t_cycles += 5;
			break;

			// AND

			case 0x29:
				immediate();
				and_();
				t_cycles += 2;
			break;

			case 0x25:
				zeroPage();
				and_();
				t_cycles += 4;
			break;

			case 0x35:
				zeroPage_X();
				and_();
				t_cycles += 4;
			break;

			case 0x32:
				indirect();
				and_();
				t_cycles += 7;
			break;

			case 0x21:
				indexedIndirect_X();
				and_();
				t_cycles += 7;
			break;

			case 0x31:
				indirectIndexed_Y();
				and_();
				t_cycles += 7;
			break;

			case 0x2D:
				absolute();
				and_();
				t_cycles += 5;
			break;

			case 0x3D:
				absolute_X();
				and_();
				t_cycles += 5;
			break;

			case 0x39:
				absolute_Y();
				and_();
				t_cycles += 5;
			break;


			case 0x06:
				zeroPage();
				asl();
				t_cycles += 6;
			break;

			case 0x16:
				zeroPage_X();
				asl();
				t_cycles += 6;
			break;

			case 0x0E:
				absolute();
				asl();
				t_cycles += 7;
			break;

			case 0x1E:
				absolute_X();
				asl();
				t_cycles += 7;
			break;

			case 0x0A:
				asl_a();
				t_cycles += 2;
			break;

			case 0x0F:
				zeroPage();
				bbri(0x1);
				t_cycles += 6;
			break;

			case 0x1F:
				zeroPage();
				bbri(0x2);
				t_cycles += 6;
			break;

			case 0x2F:
				zeroPage();
				bbri(0x4);
				t_cycles += 6;
			break;

			case 0x3F:
				zeroPage();
				bbri(0x8);
				t_cycles += 6;
			break;

			case 0x4F:
				zeroPage();
				bbri(0x10);
				t_cycles += 6;
			break;

			case 0x5F:
				zeroPage();
				bbri(0x20);
				t_cycles += 6;
			break;

			case 0x6F:
				zeroPage();
				bbri(0x40);
				t_cycles += 6;
			break;

			case 0x7F:	
				zeroPage();
				bbri(0x80);
				t_cycles += 6;
			break;			

			case 0x90:
				relative();
				bcc();
				t_cycles += 2;
			break;

			case 0x8F:
				zeroPage();
				bbsi(0x1);
				t_cycles += 6;
			break;

			case 0x9F:
				zeroPage();
				bbsi(0x2);	
				t_cycles += 6;		
			break;

			case 0xAF:
				zeroPage();
				bbsi(0x4);
				t_cycles += 6;
			break;

			case 0xBF:
				zeroPage();
				bbsi(0x8);
				t_cycles += 6;
			break;

			case 0xCF:
				zeroPage();
				bbsi(0x10);
				t_cycles += 6;
			break;

			case 0xDF:
				zeroPage();
				bbsi(0x20);
				t_cycles += 6;
			break;

			case 0xEF:
				zeroPage();
				bbsi(0x40);
				t_cycles += 6;
			break;

			case 0xFF:
				zeroPage();
				bbsi(0x80);
				t_cycles += 6;
			break;

			case 0xB0:
				relative();
				bcs();
				t_cycles += 2;
			break;

			case 0xF0:
				relative();
				beq();
				t_cycles += 2;
			break;

			case 0x89:
				immediate();
				bit();
				t_cycles +=2;
			break;

			case 0x24:
				zeroPage();
				bit();
				t_cycles += 4;
			break;

			case 0x34:
				zeroPage_X();
				bit();
				t_cycles += 4;
			break;

			case 0x2C:
				absolute();
				bit();
				t_cycles += 5;
			break;

			case 0x3C:
				absolute_X();
				bit();
				t_cycles += 5;
			break;


			case 0x30:
				relative();
				bmi();
				t_cycles += 2;
			break;

			case 0xD0:
				relative();
				bne();
				t_cycles += 2;
			break;

			case 0x10:
				relative();
				bpl();
				t_cycles += 2;
			break;

			case 0x80:
				relative();
				bra();
				t_cycles += 4;
			break;

			case 0x00:
				brk();
				t_cycles += 8;
			break;

			case 0x44:
				relative();
				bsr();
				t_cycles += 8;
			break;

			case 0x70:
				relative();
				bvs();
				t_cycles += 2;
			break;

			case 0x50:
				relative();
				bvc();
				t_cycles += 2;
			break;

			case 0x18:
				clc();
				t_cycles += 2;
			break;

			case 0x62:
				cla();
				t_cycles += 2;
			break;

			case 0xD8:
				cld();
				t_cycles += 2;
			break;

			case 0x58:
				cli();
				t_cycles += 2;
			break;

			case 0xB8:
				clv();
				t_cycles += 2;
			break;

			case 0xC2:
				cly();
				t_cycles += 2;
			break;

			case 0x82:
				clx();
				t_cycles += 2;
			break;

			case 0xE0:
				immediate();
				cpx();
				t_cycles += 2;
			break;

			case 0xE4:
				zeroPage();
				cpx();
				t_cycles += 4;
			break;

			case 0xEC:
				absolute();
				cpx();
				t_cycles += 5;
			break;

			// TODO: Number of cycles to these instructions
			// On the documentation which I Am reading(http://emu-docs.org/PC%20Engine/pce_doc/pce_cpu.html) only
			// has ?? on the cycles, maybe it doesn't care ? :|
			case 0xD4:
				csh();
				t_cycles += 2;
			break;

			case 0x54:
				csl();
				t_cycles += 2;
			break;

			case 0xC9:
				immediate();
				cmp();
				t_cycles += 2;
			break;

			case 0xC5:
				zeroPage();
				cmp();
				t_cycles += 4;
			break;

			case 0xD5:
				zeroPage_X();
				cmp();
				t_cycles += 4;
			break;

			case 0xD2:
				indirect();
				cmp();
				t_cycles += 4;
			break;

			case 0xC1:
				indexedIndirect_X();
				cmp();
				t_cycles += 7;
			break;

			case 0xD1:
				indirectIndexed_Y();
				cmp();
				t_cycles += 7;
			break;

			case 0xCD:
				absolute();
				cmp();
				t_cycles += 5;
			break;

			case 0xDD:
				absolute_X();
				cmp();
				t_cycles += 5;
			break;

			case 0xD9:
				absolute_Y();
				cmp();
				t_cycles += 5;
			break;

			case 0xCA:
				dex();
				t_cycles += 2;
			break;

			case 0xC6:
				zeroPage();
				dec();
				t_cycles += 6;
			break;

			case 0xD6:
				zeroPage_X();
				dec();
				t_cycles += 6;
			break;

			case 0xCE:
				absolute();
				dec();
				t_cycles += 7;
			break;

			case 0xDE:
				absolute_X();
				dec();
				t_cycles += 7;
			break;

			case 0x3A:
				dec_a();
				t_cycles += 2;
			break;

			case 0xC0:
				immediate();
				cpy();
				t_cycles += 2;
			break;

			case 0xC4:
				zeroPage();
				cpy();
				t_cycles += 4;
			break;

			case 0xCC:
				absolute();
				cpy();
				t_cycles += 5;
			break;

			case 0x49:
				immediate();
				eor();
				t_cycles += 2;
			break;

			case 0x45:
				zeroPage();
				eor();
				t_cycles += 4;
			break;

			case 0x55:
				zeroPage_X();
				eor();
				t_cycles += 4;
			break;

			case 0x52:
				indirect();
				eor();
				t_cycles += 7;
			break;

			case 0x41:
				indexedIndirect_X();
				eor();
				t_cycles += 7;
			break;

			case 0x51:
				indirectIndexed_Y();
				eor();
				t_cycles += 7;
			break;

			case 0x4D:
				absolute();
				eor();
				t_cycles += 5;
			break;

			case 0x5D:
				absolute_X();
				eor();
				t_cycles += 5;
			break;

			case 0x59:
				absolute_Y();
				eor();
				t_cycles += 5;
			break;

			case 0xE6:
				zeroPage();
				inc();
				t_cycles += 6;
			break;

			case 0xF6:
				zeroPage_X();
				inc();
				t_cycles += 6;
			break;

			case 0xEE:
				absolute();
				inc();
				t_cycles += 7;
			break;

			case 0xFE:
				absolute_X();
				inc();
				t_cycles += 7;
			break;

			case 0x1A:
				inc_a();
				t_cycles += 2;
			break;

			case 0xE8:
				inx();
				t_cycles += 2;
			break;

			case 0x88:
				dey();
				t_cycles += 2;
			break;

			case 0xC8:
				iny();
				t_cycles += 2;
			break;

			case 0x4C:
				absolute();
				jmp();
				t_cycles += 4;
			break;

			case 0x6C:
				indirect();
				jmp();
				t_cycles += 7;
			break;

			case 0x7C:
				absoluteIndirect_X();
				jmp();
				t_cycles += 7;
			break;

			case 0x20:
				absolute();
				jsr();
				t_cycles += 7;
			break;

			case 0xA9:
				immediate();
				lda();
				t_cycles += 2;
			break;

			case 0xA5:
				zeroPage();
				lda();
				t_cycles += 4;
			break;

			case 0xB5:
				zeroPage_X();
				lda();
				t_cycles += 4;
			break;

			case 0xB2:
				indirect();
				lda();
				t_cycles += 7;
			break;

			case 0xA1:
				indexedIndirect_X();
				lda();
				t_cycles += 7;
			break;

			case 0xB1:
				indirectIndexed_Y();
				lda();
				t_cycles += 7;
			break;

			case 0xAD:
				absolute();
				lda();
				t_cycles += 5;
			break;

			case 0xBD:
				absolute_X();
				lda();
				t_cycles += 5;
			break;

			case 0xB9:
				absolute_Y();
				lda();
				t_cycles += 5;
			break;

			case 0xA2:
				immediate();
				ldx();
				t_cycles += 2;
			break;

			case 0xA6:
				zeroPage();
				ldx();
				t_cycles += 4;
			break;

			case 0xB6:
				zeroPage_X();
				ldx();
				t_cycles += 4;
			break;

			case 0xAE:
				absolute();
				ldx();
				t_cycles += 5;
			break;

			case 0xBE:
				absolute_Y();
				ldx();
				t_cycles += 5;
			break;

			case 0xA0:
				immediate();
				ldy();
				t_cycles += 2;
			break;

			case 0xA4:
				zeroPage();
				ldy();
				t_cycles += 4;
			break;

			case 0xB4:
				zeroPage_X();
				ldy();
				t_cycles += 4;
			break;

			case 0xAC:
				absolute();
				ldy();
				t_cycles += 5;
			break;

			case 0xBC:
				absolute_X();
				ldy();
				t_cycles += 5;
			break;

			case 0x46:
				zeroPage();
				lsr();
				t_cycles += 6;
			break;

			case 0x56:
				zeroPage_X();
				lsr();
				t_cycles += 6;
			break;

			case 0x4E:
				absolute();
				lsr();
				t_cycles += 7;
			break;

			case 0x5E:
				absolute_X();
				lsr();
				t_cycles += 7;
			break;

			case 0x4A:
				lsr_a();
				t_cycles += 2;
			break;

			case 0x09:
				immediate();
				ora();
				t_cycles += 2;
			break;

			case 0x05:
				zeroPage();
				ora();
				t_cycles += 4;
			break;

			case 0x15:
				zeroPage_X();
				ora();
				t_cycles += 4;
			break;

			case 0x12:
				indirect();
				ora();
				t_cycles += 7;
			break;

			case 0x01:
				indexedIndirect_X();
				ora();
				t_cycles += 7;
			break;

			case 0x11:
				indirectIndexed_Y();
				ora();
				t_cycles += 7;
			break;

			case 0x0D:
				absolute();
				ora();
				t_cycles += 5;
			break;

			case 0x1D:
				absolute_X();
				ora();
				t_cycles += 5;
			break;

			case 0x19:
				absolute_Y();
				ora();
				t_cycles += 5;
			break;

			// NOP
			case 0xEA:
				t_cycles += 2;
			break;

			case 0x48:
				pha();
				t_cycles += 3;
			break;	

			case 0x08:
				php();
				t_cycles += 3;
			break;

			case 0xDA:
				phx();
				t_cycles += 3;
			break;

			case 0x5A:
				phy();
				t_cycles += 3;
			break;

			case 0x68:
				pla();
				t_cycles += 4;
			break;

			case 0x28:
				plp();
				t_cycles += 4;
			break;

			case 0xFA:
				plx();
				t_cycles += 4;
			break;

			case 0x7A:
				ply();
				t_cycles += 4;
			break;

			case 0x26:
				zeroPage();
				rol();
				t_cycles += 6;
			break;

			case 0x36:
				zeroPage_X();
				rol();
				t_cycles += 6;
			break;

			case 0x2E:
				absolute();
				rol();
				t_cycles += 7;
			break;

			case 0x3E:
				absolute_X();
				rol();
				t_cycles += 7;
			break;

			case 0x2A:
				rol_a();
				t_cycles += 2;
			break;

			case 0x07:
				zeroPage();
				rmbi(0x1);
				t_cycles += 7;
			break;

			case 0x17:
				zeroPage();
				rmbi(0x2);
				t_cycles += 7;
			break;

			case 0x27:
				zeroPage();
				rmbi(0x4);
				t_cycles += 7;
			break;

			case 0x37:
				zeroPage();
				rmbi(0x6);
				t_cycles += 7;
			break;

			case 0x47:
				zeroPage();
				rmbi(0x10);
				t_cycles += 7;
			break;

			case 0x57:
				zeroPage();
				rmbi(0x20);
				t_cycles += 7;
			break;

			case 0x67:
				zeroPage();
				rmbi(0x40);
				t_cycles += 7;
			break;

			case 0x77:
				zeroPage();
				rmbi(0x80);
				t_cycles += 7;
			break;

			case 0x66:
				zeroPage();
				ror();
				t_cycles += 6;
			break;

			case 0x76:
				zeroPage_X();
				ror();
				t_cycles += 6;
			break;

			case 0x6E:
				absolute();
				ror();
				t_cycles += 7;
			break;

			case 0x7E:
				absolute_X();
				ror();
				t_cycles += 7;
			break;

			case 0x6A:
				ror_a();
				t_cycles += 2;
			break;

			case 0x40:
				rti();
				t_cycles += 7;
			break;

			case 0x60:
				rts();
				t_cycles += 7;
			break;

			case 0x22:
				sax();
				t_cycles += 3;
			break;

			case 0x42:
				say();
				t_cycles += 3;
			break;

			case 0xE9:
				immediate();
				sbc();
				t_cycles += 2;
			break;

			case 0xE5:
				zeroPage();
				sbc();
				t_cycles += 4;
			break;

			case 0xF5:
				zeroPage_X();
				sbc();
				t_cycles += 4;
			break;

			case 0xF2:
				indirect();
				sbc();
				t_cycles += 7;
			break;

			case 0xE1:
				indexedIndirect_X();
				sbc();
				t_cycles += 7;
			break;

			case 0xF1:
				indirectIndexed_Y();
				sbc();
				t_cycles += 7;
			break;

			case 0xED:
				absolute();
				sbc();
				t_cycles += 5;
			break;

			case 0xFD:
				absolute_X();
				sbc();
				t_cycles += 5;
			break;

			case 0xF9:
				absolute_Y();
				sbc();
				t_cycles += 5;
			break;

			case 0xF8:
				sed();
				t_cycles += 2;
			break;

			case 0x38:
				sec();
				t_cycles += 2;
			break;

			case 0x78:
				sei();
				t_cycles += 2;
			break;

			case 0xF4:
				set();
				t_cycles += 2;
			break;

			case 0x03:
				st0();
				t_cycles += 4;
			break;

			case 0x13:
				st1();
				t_cycles += 4;
			break;

			case 0x23:
				st2();
				t_cycles += 4;
			break;

			case 0x87:
				zeroPage();
				smbi(0x1);
				t_cycles += 7;
			break;

			case 0x97:
				zeroPage();
				smbi(0x2);
				t_cycles += 7;				
			break;

			case 0xA7:
				zeroPage();
				smbi(0x4);
				t_cycles += 7;
			break;

			case 0xB7:
				zeroPage();
				smbi(0x8);
				t_cycles += 7;
			break;

			case 0xC7:
				zeroPage();
				smbi(0x10);
				t_cycles += 7;
			break;

			case 0xD7:
				zeroPage();
				smbi(0x20);
				t_cycles += 7;
			break;

			case 0xE7:
				zeroPage();
				smbi(0x40);
				t_cycles += 7;
			break;

			case 0xF7:
				zeroPage();
				smbi(0x80);
				t_cycles += 7;
			break;

			case 0x85:
				zeroPage();
				sta();
				t_cycles += 4;
			break;

			case 0x95:
				zeroPage_X();
				sta();
				t_cycles += 4;
			break;

			case 0x92:
				indirect();
				sta();
				t_cycles += 7;
			break;

			case 0x81:
				indexedIndirect_X();
				sta();
				t_cycles += 7;
			break;

			case 0x91:
				indirectIndexed_Y();
				sta();
				t_cycles += 7;
			break;

			case 0x8D:
				absolute();
				sta();
				t_cycles += 5;
			break;

			case 0x9D:
				absolute_X();
				sta();
				t_cycles += 5;
			break;	

			case 0x99:
				absolute_Y();
				sta();
				t_cycles += 5;
			break;

			case 0x86:
				zeroPage();
				stx();
				t_cycles += 4;
			break;

			case 0x96:
				zeroPage_Y();
				stx();
				t_cycles += 4;
			break;

			case 0x8E:
				absolute();
				t_cycles += 5;
			break;

			case 0x84:
				zeroPage();
				t_cycles += 4;
			break;

			case 0x94:
				zeroPage_X();
				t_cycles += 4;
			break;

			case 0x8C:
				absolute();
				t_cycles += 5;
			break;

			case 0x64:
				zeroPage();
				stz();
				t_cycles += 4;
			break;

			case 0x74:
				zeroPage_X();
				stz();
				t_cycles += 4;
			break;

			case 0x9C:
				absolute();
				stz();
				t_cycles += 5;
			break;

			case 0x9E:
				absolute_X();
				stz();
				t_cycles += 5;
			break;

			case 0xF3:
				tai();
			break;

			case 0x02:
				sxy();
				t_cycles += 3;
			break;

			case 0x53:
				tami();
				t_cycles += 5;
			break;

			case 0xAA:
				tax();
				t_cycles += 2;
			break;

			case 0xA8:
				tay();
				t_cycles += 2;
			break;

			case 0xE3:
				tia();
			break;

			case 0xC3:
				tdd();
			break;

			case 0xD3:
				tin();
			break;

			case 0x73:
				tii();
			break;

			case 0x43:
				tmai();
			break;

			case 0x14:
				zeroPage();
				trb();
				t_cycles += 6;
			break;

			case 0x1C:
				absolute();
				trb();
				t_cycles += 7;
			break;

			case 0x04:
				zeroPage();
				tsb();
				t_cycles += 6;
			break;

			case 0x0C:
				absolute();
				tsb();
				t_cycles += 7;
			break;

			case 0x83:
				immediate();
				addrReg_2 = memory.readMemory(pc++);
				tst();
				t_cycles += 7;
			break;

			case 0xA3:
				immediate();
				addrReg_2 = memory.readMemory(pc++);
				addrReg_2 = ((x + addrReg_2) & 0xFF);
				tst();
				t_cycles += 7;
			break;

			case 0x93:
				immediate();
				addrReg_2= (memory.readMemory(pc) | (memory.readMemory(pc+1) << 8));
				pc += 2;
				tst();
				t_cycles += 8;
			break;

			case 0xB3:
				immediate();
				addrReg_2 = (memory.readMemory(pc) | (memory.readMemory(pc+1) << 8)) + x;
				pc +=2;
				tst();
				t_cycles += 8;
			break;	

			case 0xBA:
				tsx();
				t_cycles += 2;
			break;

			case 0x8A:
				txa();
				t_cycles += 2;
			break;

			case 0x98:
				tya();
				t_cycles += 2;
			break;

			case 0x9A:
				txs();
				t_cycles += 2;
			break;

			default:
				printf("Invalid Opcode: 0x%X\n", opcode);
			break;
		}
	}	
}

