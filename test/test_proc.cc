#define NPROC 4
#define NCAPS 32
#include <assert.h>
extern "C" {
#include "mockup.c"
#include "../src/cap.c"
#include "../src/cnode.c"
#include "../src/proc.c"
}
#include <gtest/gtest.h>

namespace
{
class CapTest : public ::testing::Test
{
      protected:
	CapTest()
	{
		const union cap caps[] = {
			cap_pmp(0x205fff, CAP_RWX),
			cap_pmp(0x305fff, CAP_RX),
			cap_pmp(0x405fff, CAP_RW),
			cap_pmp(0x505fff, CAP_R),
		};
		cnode_init();
		cnode_handle_t root = cnode_get_root_handle();
		for (int i = 0; i < ARRAY_SIZE(caps); ++i) {
			cnode_handle_t handle = cnode_get_handle(0, i);
			cnode_insert(handle, caps[i], root);
		}
	}

	~CapTest() override
	{
	}

	void SetUp() override
	{
	}

	void TearDown() override
	{
	}
};
} // namespace

TEST_F(CapTest, LoadPmp)
{
	processes[0].regs[REG_PMP] = 0x03020100ull;
	proc_load_pmp(&processes[0]);
	EXPECT_EQ(csrr_pmpcfg0(), 0x1F1F1F1F191B1D1Full);
	EXPECT_EQ(csrr_pmpaddr0(), 0x205FFFull);
	EXPECT_EQ(csrr_pmpaddr1(), 0x305FFFull);
	EXPECT_EQ(csrr_pmpaddr2(), 0x405FFFull);
	EXPECT_EQ(csrr_pmpaddr3(), 0x505FFFull);
	EXPECT_EQ(csrr_pmpaddr4(), 0x205FFFull);
	EXPECT_EQ(csrr_pmpaddr5(), 0x205FFFull);
	EXPECT_EQ(csrr_pmpaddr6(), 0x205FFFull);
	EXPECT_EQ(csrr_pmpaddr7(), 0x205FFFull);
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}