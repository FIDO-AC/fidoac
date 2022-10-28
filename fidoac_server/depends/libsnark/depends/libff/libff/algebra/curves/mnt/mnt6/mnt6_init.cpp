/** @file
 *****************************************************************************

 Implementation of interfaces for initializing MNT6.

 See mnt6_init.hpp .

 *****************************************************************************
 * @author     This file is part of libff, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#include <libff/algebra/curves/mnt/mnt6/mnt6_g1.hpp>
#include <libff/algebra/curves/mnt/mnt6/mnt6_g2.hpp>
#include <libff/algebra/curves/mnt/mnt6/mnt6_init.hpp>

namespace libff
{

// bigint<mnt6_r_limbs> mnt6_modulus_r = mnt46_modulus_B;
// bigint<mnt6_q_limbs> mnt6_modulus_q = mnt46_modulus_A;

mnt6_Fq3 mnt6_twist;
mnt6_Fq3 mnt6_twist_coeff_a;
mnt6_Fq3 mnt6_twist_coeff_b;
mnt6_Fq mnt6_twist_mul_by_a_c0;
mnt6_Fq mnt6_twist_mul_by_a_c1;
mnt6_Fq mnt6_twist_mul_by_a_c2;
mnt6_Fq mnt6_twist_mul_by_b_c0;
mnt6_Fq mnt6_twist_mul_by_b_c1;
mnt6_Fq mnt6_twist_mul_by_b_c2;
mnt6_Fq mnt6_twist_mul_by_q_X;
mnt6_Fq mnt6_twist_mul_by_q_Y;

bigint<mnt6_q_limbs> mnt6_ate_loop_count;
bool mnt6_ate_is_loop_count_neg;
bigint<6 * mnt6_q_limbs> mnt6_final_exponent;
bigint<mnt6_q_limbs> mnt6_final_exponent_last_chunk_abs_of_w0;
bool mnt6_final_exponent_last_chunk_is_w0_neg;
bigint<mnt6_q_limbs> mnt6_final_exponent_last_chunk_w1;

void init_mnt6_params()
{
    typedef bigint<mnt6_r_limbs> bigint_r;
    typedef bigint<mnt6_q_limbs> bigint_q;

    assert(
        sizeof(mp_limb_t) == 8 ||
        sizeof(mp_limb_t) == 4); // Montgomery assumes this

    /* parameters for scalar field Fr */
    mnt6_modulus_r = bigint_r("475922286169261325753349249653048451545124879242"
                              "694725395555128576210262817955800483758081");
    assert(mnt6_Fr::modulus_is_valid());
    if (sizeof(mp_limb_t) == 8) {
        mnt6_Fr::Rsquared =
            bigint_r("273000478523237720910981655601160860640083126627235719712"
                     "980612296263966512828033847775776");
        mnt6_Fr::Rcubed =
            bigint_r("427298980065529822574935274648041073124704261331681436071"
                     "990730954930769758106792920349077");
        mnt6_Fr::inv = 0xb071a1b67165ffff;
    }
    if (sizeof(mp_limb_t) == 4) {
        mnt6_Fr::Rsquared =
            bigint_r("273000478523237720910981655601160860640083126627235719712"
                     "980612296263966512828033847775776");
        mnt6_Fr::Rcubed =
            bigint_r("427298980065529822574935274648041073124704261331681436071"
                     "990730954930769758106792920349077");
        mnt6_Fr::inv = 0x7165ffff;
    }
    mnt6_Fr::num_bits = 298;
    mnt6_Fr::euler = bigint_r("237961143084630662876674624826524225772562439621"
                              "347362697777564288105131408977900241879040");
    mnt6_Fr::s = 17;
    mnt6_Fr::t = bigint_r("3630998887399759870554727551674258816109656366292531"
                          "779446068791017229177993437198515");
    mnt6_Fr::t_minus_1_over_2 =
        bigint_r("1815499443699879935277363775837129408054828183146265889723034"
                 "395508614588996718599257");
    mnt6_Fr::multiplicative_generator = mnt6_Fr("17");
    mnt6_Fr::root_of_unity =
        mnt6_Fr("26470625057180008075806930236965430553012567552126397603405487"
                "8017580902343339784464690243");
    mnt6_Fr::nqr = mnt6_Fr("17");
    mnt6_Fr::nqr_to_t = mnt6_Fr("2647062505718000807580693023696543055301256755"
                                "21263976034054878017580902343339784464690243");
    mnt6_Fr::static_init();

    /* parameters for base field Fq */
    mnt6_modulus_q = bigint_q("475922286169261325753349249653048451545124878552"
                              "823515553267735739164647307408490559963137");
    assert(mnt6_Fq::modulus_is_valid());
    if (sizeof(mp_limb_t) == 8) {
        mnt6_Fq::Rsquared =
            bigint_q("163983144722506446826715124368972380525894397127205577781"
                     "234305496325861831001705438796139");
        mnt6_Fq::Rcubed =
            bigint_q("207236281459091063710247635236340312578688659363066707916"
                     "716212805695955118593239854980171");
        mnt6_Fq::inv = 0xbb4334a3ffffffff;
    }
    if (sizeof(mp_limb_t) == 4) {
        mnt6_Fq::Rsquared =
            bigint_q("163983144722506446826715124368972380525894397127205577781"
                     "234305496325861831001705438796139");
        mnt6_Fq::Rcubed =
            bigint_q("207236281459091063710247635236340312578688659363066707916"
                     "716212805695955118593239854980171");
        mnt6_Fq::inv = 0xffffffff;
    }
    mnt6_Fq::num_bits = 298;
    mnt6_Fq::euler = bigint_q("237961143084630662876674624826524225772562439276"
                              "411757776633867869582323653704245279981568");
    mnt6_Fq::s = 34;
    mnt6_Fq::t = bigint_q("2770232305450256248897344628657729199302411164115319"
                          "9339359284829066871159442729");
    mnt6_Fq::t_minus_1_over_2 =
        bigint_q("1385116152725128124448672314328864599651205582057659966967964"
                 "2414533435579721364");
    mnt6_Fq::multiplicative_generator = mnt6_Fq("10");
    mnt6_Fq::root_of_unity =
        mnt6_Fq("12063881782691317345876882948569009984537700803089161801010977"
                "2937363554409782252579816313");
    mnt6_Fq::nqr = mnt6_Fq("5");
    mnt6_Fq::nqr_to_t = mnt6_Fq("4062206042430904010564294587302981459372625525"
                                "08985450684842547562990900634752279902740880");
    mnt6_Fq::static_init();

    /* parameters for twist field Fq3 */
    mnt6_Fq3::euler = bigint<3 * mnt6_q_limbs>(
        "5389868017855495171539724515479603613946389158900147862919313636912491"
        "5637741423690184935056189295242736833704290747216410090671804540908400"
        "2107789344621296256462630953983234857955575512841902241668515716158341"
        "94321908328559167529729507439069424158411618728014749106176");
    mnt6_Fq3::s = 34;
    mnt6_Fq3::t = bigint<3 * mnt6_q_limbs>(
        "6274632199033507112809136178669989590936327770934612330653836993631547"
        "7403976749268110067416202853483540045218880692515999649967770721889566"
        "8755040206738394052328810740708414066996862544726932237004530285669423"
        "1080113482726640944570478452261237446033817102203");
    mnt6_Fq3::t_minus_1_over_2 = bigint<3 * mnt6_q_limbs>(
        "3137316099516753556404568089334994795468163885467306165326918496815773"
        "8701988374634055033708101426741770022609440346257999824983885360944783"
        "4377520103369197026164405370354207033498431272363466118502265142834711"
        "5540056741363320472285239226130618723016908551101");
    mnt6_Fq3::non_residue = mnt6_Fq("5");
    mnt6_Fq3::nqr = mnt6_Fq3(mnt6_Fq("5"), mnt6_Fq("0"), mnt6_Fq("0"));
    mnt6_Fq3::nqr_to_t = mnt6_Fq3(
        mnt6_Fq("15436144967878350507698415627597793765433110336117446963234623"
                "0549735979552469642799720052"),
        mnt6_Fq("0"),
        mnt6_Fq("0"));
    mnt6_Fq3::Frobenius_coeffs_c1[0] = mnt6_Fq("1");
    mnt6_Fq3::Frobenius_coeffs_c1[1] =
        mnt6_Fq("47173889896752102913304085131844916599730410872955897377007731"
                "9830005517129946578866686956");
    mnt6_Fq3::Frobenius_coeffs_c1[2] =
        mnt6_Fq("41833872017402966203083983345992855478207698232645417831904159"
                "09159130177461911693276180");
    mnt6_Fq3::Frobenius_coeffs_c2[0] = mnt6_Fq("1");
    mnt6_Fq3::Frobenius_coeffs_c2[1] =
        mnt6_Fq("41833872017402966203083983345992855478207698232645417831904159"
                "09159130177461911693276180");
    mnt6_Fq3::Frobenius_coeffs_c2[2] =
        mnt6_Fq("47173889896752102913304085131844916599730410872955897377007731"
                "9830005517129946578866686956");

    /* parameters for Fq6 */
    mnt6_Fq6::non_residue = mnt6_Fq("5");
    mnt6_Fq6::Frobenius_coeffs_c1[0] = mnt6_Fq("1");
    mnt6_Fq6::Frobenius_coeffs_c1[1] =
        mnt6_Fq("47173889896752102913304085131844916599730410872955897377007731"
                "9830005517129946578866686957");
    mnt6_Fq6::Frobenius_coeffs_c1[2] =
        mnt6_Fq("47173889896752102913304085131844916599730410872955897377007731"
                "9830005517129946578866686956");
    mnt6_Fq6::Frobenius_coeffs_c1[3] =
        mnt6_Fq("47592228616926132575334924965304845154512487855282351555326773"
                "5739164647307408490559963136");
    mnt6_Fq6::Frobenius_coeffs_c1[4] =
        mnt6_Fq("41833872017402966203083983345992855478207698232645417831904159"
                "09159130177461911693276180");
    mnt6_Fq6::Frobenius_coeffs_c1[5] =
        mnt6_Fq("41833872017402966203083983345992855478207698232645417831904159"
                "09159130177461911693276181");
    mnt6_Fq6::my_Fp2::non_residue = mnt6_Fq3::non_residue;

    /* choice of short Weierstrass curve and its twist */
    mnt6_G1::coeff_a = mnt6_Fq("11");
    mnt6_G1::coeff_b = mnt6_Fq("10670008051085173567796731963258535225645425120"
                               "1367587890185989362936000262606668469523074");
    mnt6_twist = mnt6_Fq3(mnt6_Fq::zero(), mnt6_Fq::one(), mnt6_Fq::zero());
    mnt6_twist_coeff_a =
        mnt6_Fq3(mnt6_Fq::zero(), mnt6_Fq::zero(), mnt6_G1::coeff_a);
    mnt6_twist_coeff_b = mnt6_Fq3(
        mnt6_G1::coeff_b * mnt6_Fq3::non_residue,
        mnt6_Fq::zero(),
        mnt6_Fq::zero());
    mnt6_G2::twist = mnt6_twist;
    mnt6_G2::coeff_a = mnt6_twist_coeff_a;
    mnt6_G2::coeff_b = mnt6_twist_coeff_b;
    mnt6_twist_mul_by_a_c0 = mnt6_G1::coeff_a * mnt6_Fq3::non_residue;
    mnt6_twist_mul_by_a_c1 = mnt6_G1::coeff_a * mnt6_Fq3::non_residue;
    mnt6_twist_mul_by_a_c2 = mnt6_G1::coeff_a;
    mnt6_twist_mul_by_b_c0 = mnt6_G1::coeff_b * mnt6_Fq3::non_residue;
    mnt6_twist_mul_by_b_c1 = mnt6_G1::coeff_b * mnt6_Fq3::non_residue;
    mnt6_twist_mul_by_b_c2 = mnt6_G1::coeff_b * mnt6_Fq3::non_residue;
    mnt6_twist_mul_by_q_X =
        mnt6_Fq("41833872017402966203083983345992855478207698232645417831904159"
                "09159130177461911693276180");
    mnt6_twist_mul_by_q_Y =
        mnt6_Fq("47592228616926132575334924965304845154512487855282351555326773"
                "5739164647307408490559963136");

    /* choice of group G1 */
    // Identities
    mnt6_G1::G1_zero =
        mnt6_G1(mnt6_Fq::zero(), mnt6_Fq::one(), mnt6_Fq::zero());
    mnt6_G1::G1_one = mnt6_G1(
        mnt6_Fq("33668575288308222810928984635393710418569820937140417834296883"
                "8739115829740084426881123453"),
        mnt6_Fq("40259629013978098970933270771656892077762203207376274986234237"
                "4583908837063963736098549800"),
        mnt6_Fq::one());

    // Cofactor
    mnt6_G1::h = bigint<mnt6_G1::h_limbs>("1");

    // WNAF
    mnt6_G1::wnaf_window_table.resize(0);
    mnt6_G1::wnaf_window_table.push_back(11);
    mnt6_G1::wnaf_window_table.push_back(24);
    mnt6_G1::wnaf_window_table.push_back(60);
    mnt6_G1::wnaf_window_table.push_back(127);

    mnt6_G1::fixed_base_exp_window_table.resize(0);
    // window 1 is unbeaten in [-inf, 3.96]
    mnt6_G1::fixed_base_exp_window_table.push_back(1);
    // window 2 is unbeaten in [3.96, 9.67]
    mnt6_G1::fixed_base_exp_window_table.push_back(4);
    // window 3 is unbeaten in [9.67, 25.13]
    mnt6_G1::fixed_base_exp_window_table.push_back(10);
    // window 4 is unbeaten in [25.13, 60.31]
    mnt6_G1::fixed_base_exp_window_table.push_back(25);
    // window 5 is unbeaten in [60.31, 146.07]
    mnt6_G1::fixed_base_exp_window_table.push_back(60);
    // window 6 is unbeaten in [146.07, 350.09]
    mnt6_G1::fixed_base_exp_window_table.push_back(146);
    // window 7 is unbeaten in [350.09, 844.54]
    mnt6_G1::fixed_base_exp_window_table.push_back(350);
    // window 8 is unbeaten in [844.54, 1839.64]
    mnt6_G1::fixed_base_exp_window_table.push_back(845);
    // window 9 is unbeaten in [1839.64, 3904.26]
    mnt6_G1::fixed_base_exp_window_table.push_back(1840);
    // window 10 is unbeaten in [3904.26, 11309.42]
    mnt6_G1::fixed_base_exp_window_table.push_back(3904);
    // window 11 is unbeaten in [11309.42, 24015.57]
    mnt6_G1::fixed_base_exp_window_table.push_back(11309);
    // window 12 is unbeaten in [24015.57, 72288.57]
    mnt6_G1::fixed_base_exp_window_table.push_back(24016);
    // window 13 is unbeaten in [72288.57, 138413.22]
    mnt6_G1::fixed_base_exp_window_table.push_back(72289);
    // window 14 is unbeaten in [138413.22, 156390.30]
    mnt6_G1::fixed_base_exp_window_table.push_back(138413);
    // window 15 is unbeaten in [156390.30, 562560.50]
    mnt6_G1::fixed_base_exp_window_table.push_back(156390);
    // window 16 is unbeaten in [562560.50, 1036742.02]
    mnt6_G1::fixed_base_exp_window_table.push_back(562560);
    // window 17 is unbeaten in [1036742.02, 2053818.86]
    mnt6_G1::fixed_base_exp_window_table.push_back(1036742);
    // window 18 is unbeaten in [2053818.86, 4370223.95]
    mnt6_G1::fixed_base_exp_window_table.push_back(2053819);
    // window 19 is unbeaten in [4370223.95, 8215703.81]
    mnt6_G1::fixed_base_exp_window_table.push_back(4370224);
    // window 20 is unbeaten in [8215703.81, 42682375.43]
    mnt6_G1::fixed_base_exp_window_table.push_back(8215704);
    // window 21 is never the best
    mnt6_G1::fixed_base_exp_window_table.push_back(0);
    // window 22 is unbeaten in [42682375.43, inf]
    mnt6_G1::fixed_base_exp_window_table.push_back(42682375);

    /* choice of group G2 */
    // Identities
    mnt6_G2::G2_zero =
        mnt6_G2(mnt6_Fq3::zero(), mnt6_Fq3::one(), mnt6_Fq3::zero());
    mnt6_G2::G2_one = mnt6_G2(
        mnt6_Fq3(
            mnt6_Fq("4214564357728118462568265615939083222885091154891199075603"
                    "82401870203318738334702321297427"),
            mnt6_Fq("1030729274385485024635270099613449150211675847064399454049"
                    "59058962657261178393635706405114"),
            mnt6_Fq("1430291721437318526270029263247351838097683633011490092048"
                    "49580478324784395590388826052558")),
        mnt6_Fq3(
            mnt6_Fq("4646735966686894631300992275756395125412181334453888693838"
                    "93594087634649237515554342751377"),
            mnt6_Fq("1006429075019773751845750759671180718078211179601527433356"
                    "03284583254620685343989304941678"),
            mnt6_Fq("1230198555029698960269405457158411813002751801572880446630"
                    "51565390506010149881373807142903")),
        mnt6_Fq3::one());

    // Cofactor
    mnt6_G2::h = bigint<mnt6_G2::h_limbs>(
        "2265020224725762701964986904983084617918287627326025861622075353519602"
        "7008271269497733337236154908221451925226173504813188901850140437785678"
        "6623430385820659037970876666767495659520");

    // WNAF
    mnt6_G2::wnaf_window_table.resize(0);
    mnt6_G2::wnaf_window_table.push_back(5);
    mnt6_G2::wnaf_window_table.push_back(15);
    mnt6_G2::wnaf_window_table.push_back(39);
    mnt6_G2::wnaf_window_table.push_back(109);

    mnt6_G2::fixed_base_exp_window_table.resize(0);
    // window 1 is unbeaten in [-inf, 4.25]
    mnt6_G2::fixed_base_exp_window_table.push_back(1);
    // window 2 is unbeaten in [4.25, 10.22]
    mnt6_G2::fixed_base_exp_window_table.push_back(4);
    // window 3 is unbeaten in [10.22, 24.85]
    mnt6_G2::fixed_base_exp_window_table.push_back(10);
    // window 4 is unbeaten in [24.85, 60.06]
    mnt6_G2::fixed_base_exp_window_table.push_back(25);
    // window 5 is unbeaten in [60.06, 143.61]
    mnt6_G2::fixed_base_exp_window_table.push_back(60);
    // window 6 is unbeaten in [143.61, 345.66]
    mnt6_G2::fixed_base_exp_window_table.push_back(144);
    // window 7 is unbeaten in [345.66, 818.56]
    mnt6_G2::fixed_base_exp_window_table.push_back(346);
    // window 8 is unbeaten in [818.56, 1782.06]
    mnt6_G2::fixed_base_exp_window_table.push_back(819);
    // window 9 is unbeaten in [1782.06, 4002.45]
    mnt6_G2::fixed_base_exp_window_table.push_back(1782);
    // window 10 is unbeaten in [4002.45, 10870.18]
    mnt6_G2::fixed_base_exp_window_table.push_back(4002);
    // window 11 is unbeaten in [10870.18, 18022.51]
    mnt6_G2::fixed_base_exp_window_table.push_back(10870);
    // window 12 is unbeaten in [18022.51, 43160.74]
    mnt6_G2::fixed_base_exp_window_table.push_back(18023);
    // window 13 is unbeaten in [43160.74, 149743.32]
    mnt6_G2::fixed_base_exp_window_table.push_back(43161);
    // window 14 is never the best
    mnt6_G2::fixed_base_exp_window_table.push_back(0);
    // window 15 is unbeaten in [149743.32, 551844.13]
    mnt6_G2::fixed_base_exp_window_table.push_back(149743);
    // window 16 is unbeaten in [551844.13, 1041827.91]
    mnt6_G2::fixed_base_exp_window_table.push_back(551844);
    // window 17 is unbeaten in [1041827.91, 1977371.53]
    mnt6_G2::fixed_base_exp_window_table.push_back(1041828);
    // window 18 is unbeaten in [1977371.53, 3703619.51]
    mnt6_G2::fixed_base_exp_window_table.push_back(1977372);
    // window 19 is unbeaten in [3703619.51, 7057236.87]
    mnt6_G2::fixed_base_exp_window_table.push_back(3703620);
    // window 20 is unbeaten in [7057236.87, 38554491.67]
    mnt6_G2::fixed_base_exp_window_table.push_back(7057237);
    // window 21 is never the best
    mnt6_G2::fixed_base_exp_window_table.push_back(0);
    // window 22 is unbeaten in [38554491.67, inf]
    mnt6_G2::fixed_base_exp_window_table.push_back(38554492);

    /* pairing parameters */
    mnt6_ate_loop_count =
        bigint_q("689871209842287392837045615510547309923794944");
    mnt6_ate_is_loop_count_neg = true;
    mnt6_final_exponent = bigint<6 * mnt6_q_limbs>(
        "2441632013809050969789059541431343876835397748986254393590401071543906"
        "6975957855922532159264213056712140358746422742237328406558352706591021"
        "6422306180605028554512640453974447931868761990152567816487468886255270"
        "7546606307501130780086217376423631134210521168112142693161684363521585"
        "2236649271569251468773714424208521977615548771268520882870120900360322"
        "0442188067120277293518453076904749855025875277538472001305920580983636"
        "41559341826790559426614919168");
    mnt6_final_exponent_last_chunk_abs_of_w0 =
        bigint_q("689871209842287392837045615510547309923794944");
    mnt6_final_exponent_last_chunk_is_w0_neg = true;
    mnt6_final_exponent_last_chunk_w1 = bigint_q("1");
}

} // namespace libff
