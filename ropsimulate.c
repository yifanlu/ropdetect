#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define ROP_SCRATCH_SPACE 1024
#define DATA_BUFFER_SIZE 16777216 // 16*L2 size
#define TYPE_DATA 0
#define TYPE_BRANCH 1
#define TYPE_MIXED 2
#if RAND_MAX < INT_MAX
#error Unsupported RAND_MAX
#endif

volatile char scratch_space[ROP_SCRATCH_SPACE];
static unsigned char random_data[DATA_BUFFER_SIZE];
static int debug_log;
static char *rop_payload;

#define LOG(fmt, ...) do { if (debug_log) fprintf(stderr, "[%s:%d] " fmt "\n", __FUNCTION__, __LINE__, __VA_ARGS__); } while (0)

static void __attribute__((naked,noreturn)) trigger_rop(void)
{
    __asm__ ("mov sp, %0\t\n"
             "pop {pc}\t\n" :: "r" (rop_payload));
}

// pure memory intensive load
// this simulates matrix multiplication
static void sim_matrix_mul(unsigned int n, unsigned int trigger)
{
    unsigned int i, j, k, sum;
    register unsigned int steps;
    register unsigned int _trigger;

    _trigger = trigger;
    steps = 0;
    for (i = 0; i < n; i++)
    {
        for (j = 0; j < n; j++)
        {
            for (k = 0; k < n; k++)
            {
                sum += random_data[i*n+k]*random_data[k*n+j];
                steps++;
                if (_trigger > 0 && steps >= _trigger)
                {
                    trigger_rop();
                }
            }
            random_data[j*n+i] = sum / n;
        }
        if ((i % 10) == 0)
        {
            LOG("steps: %u", steps);
        }
    }
}

typedef unsigned char (*func_t)(unsigned char chr);
#define GEN_FUNC(x) \
    static unsigned char __attribute__ ((noinline)) func_##x(unsigned char chr) { return (unsigned char)(x + chr); }
GEN_FUNC(0) GEN_FUNC(1) GEN_FUNC(2) GEN_FUNC(3) GEN_FUNC(4) GEN_FUNC(5) GEN_FUNC(6) GEN_FUNC(7) GEN_FUNC(8) GEN_FUNC(9) GEN_FUNC(10) GEN_FUNC(11) GEN_FUNC(12) GEN_FUNC(13) GEN_FUNC(14) GEN_FUNC(15) GEN_FUNC(16) 
GEN_FUNC(17) GEN_FUNC(18) GEN_FUNC(19) GEN_FUNC(20) GEN_FUNC(21) GEN_FUNC(22) GEN_FUNC(23) GEN_FUNC(24) GEN_FUNC(25) GEN_FUNC(26) GEN_FUNC(27) GEN_FUNC(28) GEN_FUNC(29) GEN_FUNC(30) GEN_FUNC(31) GEN_FUNC(32) 
GEN_FUNC(33) GEN_FUNC(34) GEN_FUNC(35) GEN_FUNC(36) GEN_FUNC(37) GEN_FUNC(38) GEN_FUNC(39) GEN_FUNC(40) GEN_FUNC(41) GEN_FUNC(42) GEN_FUNC(43) GEN_FUNC(44) GEN_FUNC(45) GEN_FUNC(46) GEN_FUNC(47) GEN_FUNC(48) 
GEN_FUNC(49) GEN_FUNC(50) GEN_FUNC(51) GEN_FUNC(52) GEN_FUNC(53) GEN_FUNC(54) GEN_FUNC(55) GEN_FUNC(56) GEN_FUNC(57) GEN_FUNC(58) GEN_FUNC(59) GEN_FUNC(60) GEN_FUNC(61) GEN_FUNC(62) GEN_FUNC(63) GEN_FUNC(64) 
GEN_FUNC(65) GEN_FUNC(66) GEN_FUNC(67) GEN_FUNC(68) GEN_FUNC(69) GEN_FUNC(70) GEN_FUNC(71) GEN_FUNC(72) GEN_FUNC(73) GEN_FUNC(74) GEN_FUNC(75) GEN_FUNC(76) GEN_FUNC(77) GEN_FUNC(78) GEN_FUNC(79) GEN_FUNC(80) 
GEN_FUNC(81) GEN_FUNC(82) GEN_FUNC(83) GEN_FUNC(84) GEN_FUNC(85) GEN_FUNC(86) GEN_FUNC(87) GEN_FUNC(88) GEN_FUNC(89) GEN_FUNC(90) GEN_FUNC(91) GEN_FUNC(92) GEN_FUNC(93) GEN_FUNC(94) GEN_FUNC(95) GEN_FUNC(96) 
GEN_FUNC(97) GEN_FUNC(98) GEN_FUNC(99) GEN_FUNC(100) GEN_FUNC(101) GEN_FUNC(102) GEN_FUNC(103) GEN_FUNC(104) GEN_FUNC(105) GEN_FUNC(106) GEN_FUNC(107) GEN_FUNC(108) GEN_FUNC(109) GEN_FUNC(110) GEN_FUNC(111) GEN_FUNC(112) 
GEN_FUNC(113) GEN_FUNC(114) GEN_FUNC(115) GEN_FUNC(116) GEN_FUNC(117) GEN_FUNC(118) GEN_FUNC(119) GEN_FUNC(120) GEN_FUNC(121) GEN_FUNC(122) GEN_FUNC(123) GEN_FUNC(124) GEN_FUNC(125) GEN_FUNC(126) GEN_FUNC(127) GEN_FUNC(128) 
GEN_FUNC(129) GEN_FUNC(130) GEN_FUNC(131) GEN_FUNC(132) GEN_FUNC(133) GEN_FUNC(134) GEN_FUNC(135) GEN_FUNC(136) GEN_FUNC(137) GEN_FUNC(138) GEN_FUNC(139) GEN_FUNC(140) GEN_FUNC(141) GEN_FUNC(142) GEN_FUNC(143) GEN_FUNC(144) 
GEN_FUNC(145) GEN_FUNC(146) GEN_FUNC(147) GEN_FUNC(148) GEN_FUNC(149) GEN_FUNC(150) GEN_FUNC(151) GEN_FUNC(152) GEN_FUNC(153) GEN_FUNC(154) GEN_FUNC(155) GEN_FUNC(156) GEN_FUNC(157) GEN_FUNC(158) GEN_FUNC(159) GEN_FUNC(160) 
GEN_FUNC(161) GEN_FUNC(162) GEN_FUNC(163) GEN_FUNC(164) GEN_FUNC(165) GEN_FUNC(166) GEN_FUNC(167) GEN_FUNC(168) GEN_FUNC(169) GEN_FUNC(170) GEN_FUNC(171) GEN_FUNC(172) GEN_FUNC(173) GEN_FUNC(174) GEN_FUNC(175) GEN_FUNC(176) 
GEN_FUNC(177) GEN_FUNC(178) GEN_FUNC(179) GEN_FUNC(180) GEN_FUNC(181) GEN_FUNC(182) GEN_FUNC(183) GEN_FUNC(184) GEN_FUNC(185) GEN_FUNC(186) GEN_FUNC(187) GEN_FUNC(188) GEN_FUNC(189) GEN_FUNC(190) GEN_FUNC(191) GEN_FUNC(192) 
GEN_FUNC(193) GEN_FUNC(194) GEN_FUNC(195) GEN_FUNC(196) GEN_FUNC(197) GEN_FUNC(198) GEN_FUNC(199) GEN_FUNC(200) GEN_FUNC(201) GEN_FUNC(202) GEN_FUNC(203) GEN_FUNC(204) GEN_FUNC(205) GEN_FUNC(206) GEN_FUNC(207) GEN_FUNC(208) 
GEN_FUNC(209) GEN_FUNC(210) GEN_FUNC(211) GEN_FUNC(212) GEN_FUNC(213) GEN_FUNC(214) GEN_FUNC(215) GEN_FUNC(216) GEN_FUNC(217) GEN_FUNC(218) GEN_FUNC(219) GEN_FUNC(220) GEN_FUNC(221) GEN_FUNC(222) GEN_FUNC(223) GEN_FUNC(224) 
GEN_FUNC(225) GEN_FUNC(226) GEN_FUNC(227) GEN_FUNC(228) GEN_FUNC(229) GEN_FUNC(230) GEN_FUNC(231) GEN_FUNC(232) GEN_FUNC(233) GEN_FUNC(234) GEN_FUNC(235) GEN_FUNC(236) GEN_FUNC(237) GEN_FUNC(238) GEN_FUNC(239) GEN_FUNC(240) 
GEN_FUNC(241) GEN_FUNC(242) GEN_FUNC(243) GEN_FUNC(244) GEN_FUNC(245) GEN_FUNC(246) GEN_FUNC(247) GEN_FUNC(248) GEN_FUNC(249) GEN_FUNC(250) GEN_FUNC(251) GEN_FUNC(252) GEN_FUNC(253) GEN_FUNC(254) GEN_FUNC(255)
static const func_t func_table[256] = {
    func_0,  func_1,  func_2,  func_3,  func_4,  func_5,  func_6,  func_7,  func_8,  func_9,  func_10,  func_11,  func_12,  func_13,  func_14,  func_15,  func_16,  
    func_17,  func_18,  func_19,  func_20,  func_21,  func_22,  func_23,  func_24,  func_25,  func_26,  func_27,  func_28,  func_29,  func_30,  func_31,  func_32,  
    func_33,  func_34,  func_35,  func_36,  func_37,  func_38,  func_39,  func_40,  func_41,  func_42,  func_43,  func_44,  func_45,  func_46,  func_47,  func_48,  
    func_49,  func_50,  func_51,  func_52,  func_53,  func_54,  func_55,  func_56,  func_57,  func_58,  func_59,  func_60,  func_61,  func_62,  func_63,  func_64,  
    func_65,  func_66,  func_67,  func_68,  func_69,  func_70,  func_71,  func_72,  func_73,  func_74,  func_75,  func_76,  func_77,  func_78,  func_79,  func_80,  
    func_81,  func_82,  func_83,  func_84,  func_85,  func_86,  func_87,  func_88,  func_89,  func_90,  func_91,  func_92,  func_93,  func_94,  func_95,  func_96,  
    func_97,  func_98,  func_99,  func_100,  func_101,  func_102,  func_103,  func_104,  func_105,  func_106,  func_107,  func_108,  func_109,  func_110,  func_111,  func_112,  
    func_113,  func_114,  func_115,  func_116,  func_117,  func_118,  func_119,  func_120,  func_121,  func_122,  func_123,  func_124,  func_125,  func_126,  func_127,  func_128,  
    func_129,  func_130,  func_131,  func_132,  func_133,  func_134,  func_135,  func_136,  func_137,  func_138,  func_139,  func_140,  func_141,  func_142,  func_143,  func_144,  
    func_145,  func_146,  func_147,  func_148,  func_149,  func_150,  func_151,  func_152,  func_153,  func_154,  func_155,  func_156,  func_157,  func_158,  func_159,  func_160,  
    func_161,  func_162,  func_163,  func_164,  func_165,  func_166,  func_167,  func_168,  func_169,  func_170,  func_171,  func_172,  func_173,  func_174,  func_175,  func_176,  
    func_177,  func_178,  func_179,  func_180,  func_181,  func_182,  func_183,  func_184,  func_185,  func_186,  func_187,  func_188,  func_189,  func_190,  func_191,  func_192,  
    func_193,  func_194,  func_195,  func_196,  func_197,  func_198,  func_199,  func_200,  func_201,  func_202,  func_203,  func_204,  func_205,  func_206,  func_207,  func_208,  
    func_209,  func_210,  func_211,  func_212,  func_213,  func_214,  func_215,  func_216,  func_217,  func_218,  func_219,  func_220,  func_221,  func_222,  func_223,  func_224,  
    func_225,  func_226,  func_227,  func_228,  func_229,  func_230,  func_231,  func_232,  func_233,  func_234,  func_235,  func_236,  func_237,  func_238,  func_239,  func_240,  
    func_241,  func_242,  func_243,  func_244,  func_245,  func_246,  func_247,  func_248,  func_249,  func_250,  func_251,  func_252,  func_253,  func_254,  func_255
};

// pure branch intensive load
// this simulates a lot of C++ vtable calls
static void sim_func_calls(unsigned int n, unsigned int trigger)
{
    unsigned int i;
    register unsigned int steps;
    register unsigned int _trigger;
    unsigned char c;

    c = 0;
    steps = 0;
    _trigger = trigger;
    for (i = 0; i < n; i++)
    {
        c = func_table[random_data[i%DATA_BUFFER_SIZE]](c);
        steps++;
        if (_trigger > 0 && steps >= _trigger)
        {
            trigger_rop();
        }
        if ((steps % 10000) == 0)
        {
            LOG("steps: %u", steps);
        }
    }
}

// mixed branch/data load
// this simulates many string operations
static void sim_string_ops(unsigned int n, unsigned int trigger)
{
    unsigned int i;
    char *s1, *s2;
    size_t len, len1, len2;
    register unsigned int steps;
    register unsigned int _trigger;

    _trigger = trigger;
    random_data[DATA_BUFFER_SIZE-1] = '\0';
    for (i = 0; i < n; i++)
    {
        s1 = (char *)&random_data[rand() % (n/2)];
        len1 = strnlen(s1, n/2);
        s2 = (char *)&random_data[len1 + rand() % (n/2)];
        len2 = strnlen(s2, n/2);
        len = len2 > len1 ? len1 : len2;
        steps += len2 + len1;
        if (len == 0) // ignore empty strings
        {
            i--;
            steps++;
            continue;
        }

        switch (rand() % 10)
        {
            case 0: 
                memcpy(s1, s2, len);
                break;
            case 1:
                memcpy(s2, s1, len);
                break;
            case 2:
                strncmp(s1, s2, len);
                break;
            case 3:
                memcmp(s1, s2, len);
                break;
            case 4:
                memchr(s1, s2[0], len);
                break;
            case 5:
                strchr(s1, s2[0]);
                break;
            case 6:
                strspn(s1, s2);
                break;
            case 7:
                strstr(s1, s2);
                break;
            case 8:
                strrchr(s1, s2[0]);
                break;
            case 9:
                memmove(s1, s2, len);
                break;
        }
        steps += len;
        if (_trigger > 0 && steps >= _trigger)
        {
            trigger_rop();
        }
        if ((steps % 10000) < len2 + len1 + len)
        {
            LOG("steps: %u", steps);
        }
    }
}

void __attribute__((noreturn)) capture_flag(void)
{
    fprintf(stderr, "ROP payload ended.\n");
    exit(0);
}

static void show_help(void)
{
    fprintf(stderr, "usage: ropsimulate -t type [-r seed] [-p payload]\n");
    fprintf(stderr, "           -t type    one of 'address', 'data', 'branch', 'mixed'\n");
    fprintf(stderr, "           -r seed    number used for seed\n");
    fprintf(stderr, "           -p payload file used as rop payload\n");
}

static int parse_args(int argc, const char *argv[], FILE **fp, int *type)
{
    int i;
    int seed;

    seed = time(NULL);
    *type = -1;
    debug_log = 1;
    for (i = 1; i < argc; i += 2)
    {
        if (argv[i][0] != '-' || i+1 >= argc)
        {
            fprintf(stderr, "invalid argument %s\n", argv[i]);
            show_help();
            return 1;
        }
        switch (argv[i][1])
        {
            case 't':
                if (strcmp("data", argv[i+1]) == 0)
                {
                    *type = TYPE_DATA;
                }
                else if (strcmp("branch", argv[i+1]) == 0)
                {
                    *type = TYPE_BRANCH;
                }
                else if (strcmp("mixed", argv[i+1]) == 0)
                {
                    *type = TYPE_MIXED;
                }
                else if (strcmp("address", argv[i+1]) == 0)
                {
                    fprintf(stderr, "Scratch Buffer: 0x%08X\n", scratch_space);
                    fprintf(stderr, "Scratch Size: %d\n", ROP_SCRATCH_SPACE);
                    fprintf(stderr, "Flag Capture Function: 0x%08X\n", capture_flag);
                    return 1;
                }
                else
                {
                    fprintf(stderr, "invalid type %s\n", argv[i+1]);
                    return 1;
                }
                break;
            case 'r':
                seed = atoi(argv[i+1]);
                break;
            case 'p':
                if ((*fp = fopen(argv[i+1], "rb")) == NULL)
                {
                    perror("open");
                    return 1;
                }
                break;
            default:
                fprintf(stderr, "invalid argument %s\n", argv[i]);
                show_help();
                return 1;
        }
    }
    if (*type == -1)
    {
        fprintf(stderr, "No load specified\n");
        show_help();
        return 1;
    }
    srand(seed);

    return 0;
}

int main(int argc, const char *argv[])
{
    FILE *fp;
    int type;
    size_t len, pos, k;
    unsigned int i, n, trigger;

    fp = NULL;
    if (parse_args(argc, argv, &fp, &type))
    {
        return 1;
    }

    rop_payload = NULL;
    if (fp != NULL)
    {
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if ((rop_payload = malloc(len)) == NULL)
        {
            perror("memory");
            fclose(fp);
            return 1;
        }
        while ((k = fread(rop_payload+pos, 1, len-pos, fp)) < len)
        {
            len -= k;
            pos += k;
        }
    }

    fprintf(stderr, "Generating random data...\n");
    for (i = 0; i < DATA_BUFFER_SIZE; i++)
    {
        random_data[i] = rand() % 256;
    }

    switch (type)
    {
        case TYPE_DATA:
        {
            n = (int)pow(DATA_BUFFER_SIZE, 1.0f/3.0f);
            trigger = rop_payload ? rand() % (n*n*n) : 0;
            fprintf(stderr, "Simulating data intensive load...\n");
            if (trigger > 0)
            {
                fprintf(stderr, "Triggering ROP at step %u of %u\n", trigger, n*n*n);
            }
            sim_matrix_mul(n, trigger);
            break;
        }
        case TYPE_BRANCH:
        {
            n = 10000000;
            trigger = rop_payload ? rand() % n : 0;
            fprintf(stderr, "Simulating branch intensive load...\n");
            if (trigger > 0)
            {
                fprintf(stderr, "Triggering ROP at step %u of %u\n", trigger, n);
            }
            sim_func_calls(n, trigger);
            break;
        }
        case TYPE_MIXED:
        {
            n = 1000000;
            trigger = rop_payload ? rand() % n*DATA_BUFFER_SIZE : 0;
            fprintf(stderr, "Simulating mixed load...\n");
            if (trigger > 0)
            {
                fprintf(stderr, "Triggering ROP at step %u of %u\n", trigger, n*DATA_BUFFER_SIZE);
            }
            sim_string_ops(n, trigger);
            break;
        }
    }

    return 0;
}
