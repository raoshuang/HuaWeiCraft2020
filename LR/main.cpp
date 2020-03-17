#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <climits>


#include <cfloat>
#include <cstring>

#ifndef NO_NEON
#include <arm_neon.h>
#endif

#include <set>

using namespace std;

const int features_start = 950;
const int features_num = 50;
const set<int> features_index = {};

struct Data {
    vector<float> features;
    int label;
    Data(vector<float> f, int l) : features(f), label(l)
    {}
};


class LR {
public:
    void train();

    void predict();

    LR(const string & train_file, const string & test_file, const string & predict_file);
    float fast_exp(float x);

private:
    vector<Data> train_data_;
    vector<Data> test_data_;
    vector<int> predict_vec;

    vector<float> weight_;
    vector<float> min_error_weight_;
    float min_error_rate_;
    int min_error_iter_no_;

    string weightParamFile = "modelweight.txt";

private:
    vector<Data> LoadData(const string & filename, bool last_label_exist ,int read_line_num);
    void LoadTrainData();

    void LoadTestData();

    void initParam();

    float dot (const vector<float> & vec1, const vector<float> & vec2);

    float sigmoidCalc(const float wxb);
    float lossCal(const vector<float> & weight);

    float gradientSlope(const vector<Data> & dataSet, int index, const vector<float> & sigmoidVec);
    void StorePredict();

private:
    string train_file_;
    string test_file_;
    string predict_file_;
    const int read_line_num = 1800;
private:

    const float wtInitV = 1;

    float rate_start = 0.9;
    const float decay = 0.0001;
    const float rate_min = 0.02;

    const int maxIterTimes = 200;
    const float predictTrueThresh = 0.5;
    const int train_show_step = 1;
};

inline float LR::fast_exp(float x) {
    union {uint32_t i;float f;} v;
    v.i=(1<<23)*(1.4426950409*x+126.93490512f);

    return v.f;
}


void LR::train() {

    LoadTrainData();

#ifdef TEST
    clock_t start_time = clock();
#endif

    float sigmoidVal;
    float wxbVal;

    vector<float> sigmoidVec(train_data_.size(), 0.0);

    for (int i = 0; i < maxIterTimes; i++) {

        for (int j = 0; j < train_data_.size(); j++) {
            wxbVal = dot(train_data_[j].features, weight_);
            sigmoidVal = sigmoidCalc(wxbVal);
            sigmoidVec[j] = sigmoidVal;
        }

        float error_rate = 0.0;
        for (int j = 0; j < train_data_.size(); j++) {
            error_rate += pow(train_data_[j].label - sigmoidVec[j], 2);
        }

        if (error_rate < min_error_rate_) {
            min_error_rate_ = error_rate;
            min_error_weight_ = weight_;
            min_error_iter_no_ = i;
        }


//        float rate = rate_start * 1.0 / (1.0 + decay * i);
//        rate = max(rate, rate_min);

        rate_start = rate_start * 1.0 / (1.0 + decay * i);
        float rate = max(rate_start, rate_min);

        for (int j = 0; j < weight_.size(); j++) {
            weight_[j] += rate * gradientSlope(train_data_, j, sigmoidVec);
        }

#ifdef TEST
        if (i % train_show_step == 0) {
            cout << "iter: " << i << " error_rate: " << error_rate << " rate: " << rate;
                cout << ". updated weight value is : ";
                for (int j = 0; j < weight_.size(); j++) {
                    cout << weight_[j] << "  ";
                }
            cout << endl;
        }
#endif
    }


#ifdef TEST
    clock_t end_time = clock();
    printf("模型训练时间（s）: %f \n", (double) (end_time - start_time) / CLOCKS_PER_SEC);
#endif

}

void LR::predict() {

    LoadTestData();

#ifdef TEST
    clock_t start_time = clock();
#endif

    float sigVal;
    int predictVal;

    for (int j = 0; j < test_data_.size(); j++) {
        sigVal = sigmoidCalc(dot(test_data_[j].features, min_error_weight_));
        predictVal = sigVal >= predictTrueThresh ? 1 : 0;
        predict_vec.push_back(predictVal);
    }

#ifdef TEST
    clock_t end_time = clock();
    printf("模型预测时间（s）: %f \n", (double) (end_time - start_time) / CLOCKS_PER_SEC);
#endif

    StorePredict();


#ifdef TEST
    cout << "min_error_iter_no: " << min_error_iter_no_ << " min_error_rate: " << min_error_rate_ << endl;
#endif

}

LR::LR(const string & train_file, const string & test_file, const string & predict_file):
        train_file_(train_file),
        test_file_(test_file),
        predict_file_(predict_file),
        min_error_rate_(DBL_MAX),
        min_error_iter_no_(-1) {
    initParam();
}


inline void LR::LoadTrainData() {
#ifdef TEST
    clock_t start_time = clock();
#endif
    train_data_ = LoadData(train_file_, true, read_line_num);
#ifdef TEST
    clock_t end_time = clock();
    printf("训练集读取时间（s）: %f \n", (double) (end_time - start_time) / CLOCKS_PER_SEC);
#endif
}

inline void LR::LoadTestData() {
#ifdef TEST
    clock_t start_time = clock();
#endif
    test_data_ = LoadData(test_file_, false, -1);
#ifdef TEST
    clock_t end_time = clock();
    printf("测试集读取时间（s）: %f \n", (double) (end_time - start_time) / CLOCKS_PER_SEC);
#endif
}

inline void LR::initParam()
{
//    weight_ = vector<float>(feature_indexs.size(), wtInitV);
    weight_ = vector<float>(features_num, wtInitV);
//    weight_ = vector<float>({-0.059816670384638025,-0.10407984287996272,-0.05613224322115387,-0.16979192328226153,-0.2642977423597506,-0.09064244686735004,0.2437004585500007,-0.022348809509584927,0.37756749467700634,0.11506264213409181,0.09773468891739479,-0.166941717655801,0.014979508174592742,0.20920926063046316,-0.07355145836403675,0.2645944108628651,-0.12396130482991866,0.09362855782220234,0.20697959993233384,-0.2451094724848437,-0.023450570897621762,0.03487041295444707,-0.03718859332217861,-0.09263983702600492,0.49094954169667127,0.22514451316168446,-0.007423206476026761,0.3476720468342447,0.0003274780616425686,-0.06998509029498477,-0.3031236607981309,-0.05337344493176874,0.1782205196814198,0.23462115448220577,0.0728049173472729,0.32246586406953365,0.1538331952766376,-0.06800252944768143,-0.09104800247319089,-0.061457087270520225,-0.14991779077306427,-0.0855115562283822,-0.35808533127875714,-0.1955778775174905,-0.24534731411764288,-0.06779816851719708,-0.08370393257996542,0.19352094320446955,0.2151551815688107,-0.07172343282680696,0.18480613608216964,0.09902517040353752,0.025558841353900717,0.033911785324335586,-0.02205431279059412,-0.10118659178747008,-0.17748772054508832,0.13558041018448208,0.17994689711516126,-0.2743103368173875,0.24103738408948877,-0.08187488388170248,-0.09932219854416821,-0.28916019115974023,-0.27892213626953166,0.2891975501383105,0.22377863997203687,-0.20304821463227518,-0.030862777203275644,0.12074635603879139,0.17980887051555855,-0.34785485830028307,0.25361252185687294,0.01274066718844388,0.3322599141036257,-0.0866427496230807,-0.18339758365623388,-0.10802685258042119,0.13968910923596897,-0.17237315186397817,0.029794126858438692,0.2503932217840393,-0.2161253717838868,-0.16524124122855424,-0.08689278449017912,0.0741064926059434,-0.1531597633154226,0.37429613843642945,0.1924426875929053,-0.09609201851449489,-0.14769693755392044,0.11508787408239449,0.22660072804955214,-0.1418519177670594,0.1870639249331439,-0.14319959681837424,0.22949527965281613,-0.2075634260694629,-0.3565517992643012,-0.1695324713394075,-0.0063019549630882465,-0.06184790959126476,0.2198638111969034,-0.47017930374652434,-0.0011670851777115764,0.1739073432211831,-0.07128543707010165,0.1438159916876335,0.26031842940121325,0.10774809325198442,0.2598965672231332,-0.19023194702582147,-0.01736731036967095,-0.03758904400337726,-0.3954985671472752,0.14978275460686996,-0.032339230167831176,0.04199906974686701,0.17348259672662295,0.41462027269406476,0.011548168621693309,-0.17392949166127236,0.018120494670313566,0.026212538443617272,-0.19148749942523016,-0.029370681020523367,-0.1336777718248962,0.0825787038576764,-0.0278311658500105,0.1435450464338558,0.2281590844571802,0.37971580451164744,-0.0722416643545176,-0.0780892466500272,-0.16183480413093984,-0.05156336533902138,0.2924840905092182,0.1325395264575172,-0.0799367642105289,0.051856735247074015,0.03797688952457916,0.3748320141784381,-0.07653939367786203,0.02596001435840648,0.16221913087664105,-0.022921380375232715,-0.08306468302909925,-0.14163808760682248,-0.06110863227452115,-0.1174508094975406,0.07418926584173027,0.30421997949733576,-0.008415934073584017,0.038548043939944696,-0.015482689140001865,-0.039209094598851794,0.06926437767805169,-0.1175138287150333,0.34185468160494464,-0.007994502921068687,-0.12043761536891329,0.04534884224700313,0.021033091026838223,0.033487981731437355,0.0364449535542797,-0.1332685408584805,-0.20159451967939698,0.4835886806420084,-0.2400871864459432,0.007953110954392259,0.03813511012630694,-0.2561039197459708,-0.3349595461633379,0.01616076402613801,0.009074449531765238,-0.17192422928723802,-0.05078681921997309,0.092793011761511,-0.06064120385554509,0.02460142524733191,0.13940187258614994,-0.060246394523228206,-0.03514568747731372,-0.10448401006046205,-0.15810695178144782,-0.06887771917151478,-0.2886347046637429,-0.05110367327479296,-0.29798713555461315,-0.3614414331910622,0.08238014670643168,-0.25383613712818276,0.06879136275170696,0.05815238749110338,-0.15284402916558185,-0.2971635272061262,0.36607601615266067,0.09146315241152612,0.2717681593543388,-0.16623934786864022,-0.02731775361838473,0.21938260617455,-0.23821364376441648,-0.014209742229408854,0.2833770452630149,0.028385552789739508,-0.22694775525124627,0.1324547170198279,-0.2129639268553284,-0.36434966772988736,0.09656283664459124,0.1487484198336022,0.1814205112514519,0.16467843534963245,-0.11825994111996886,0.034980259172329324,0.05474994280877633,0.12541942703954723,0.25909876427417916,0.10172926876497933,-0.11200601059893023,0.03737848789903234,-0.3233939749952499,-0.1277643317847751,-0.14440410525170674,-0.0795358369402517,0.03575291955784761,-0.1671545555339564,0.1588409248989059,-0.1054666946795378,-0.35788169671193903,0.05602097054352574,0.1339320255352094,0.04276910887424658,0.07670302572839299,0.000971305829020705,-0.10321057859607541,-0.007645650677930365,0.3230522162977428,-0.08826206210766419,-0.09887911898431959,-0.0008530526570420145,-0.05159587627970788,-0.09026425954827848,0.05524979165768547,0.219223346545336,-0.05280849646038695,0.17847835885109906,-0.07271926177615892,-0.012372166114642917,-0.0031381642591085702,0.20383237819189043,0.2685269080417898,-0.29698381351083397,-0.15658904883085606,-0.01618683897585206,-0.11360865952255024,-0.016486913179958613,-0.23359396516812905,-0.06307725734457542,-0.27951045145898085,0.15928392426753094,-0.2384032938512834,0.0850874387232998,0.32235620172994023,-0.1331059395643462,-0.03945080335788001,-0.3122915860363398,0.1282633548187263,0.07661578744936155,-0.21207081114729362,0.2521004896029566,-0.1518041170163623,-0.11331134291970076,0.1283733891452462,-0.22308083760299105,-0.01145241887200285,0.11517574906350286,0.24765747086397272,0.202426420660831,0.075245650266869,0.11698418589213874,0.1700454890168744,-0.11121149633957807,-0.016714603431557912,0.3115287657548228,0.0551910720926779,0.0865295465526183,0.1698347168477658,0.12241142110301534,-0.15271459520403077,0.035982301112432766,-0.03089060261241029,-0.21165461812150962,-0.09641291483849013,-0.06997678803569629,0.09470496084877233,0.3913997692347798,0.2645701619921269,0.1325982962296614,0.275960536845873,-0.2621010338700524,0.213671726438586,-0.2665369717998678,-0.16532136015714005,0.18018820357087098,-0.03610413549954947,0.25375114418074035,-0.24741797504970653,-0.09554572787444017,-0.16792967743458645,0.15956994153086165,0.07735869699488367,0.14277219785183373,-0.10695351840702592,0.09100106247061862,-0.09682571515882782,0.14619185986475283,0.3302180377059371,0.30545536046121036,0.14951798424341756,0.365316015861932,-0.222088671998715,-0.006881758934768337,0.05065168257200074,-0.1363677275401544,0.4240907888095397,0.2873242851463664,-0.17692117796525006,-0.20275153466263893,-0.1840122481019596,-0.036208748996004146,-0.12035877443813547,-0.28623219377623643,-0.06570907591519472,-0.06904108533566015,0.3909693701558286,0.09849165675004239,0.3496819716072857,0.3815047036120006,0.19067969790690475,-0.15932157909835828,0.17542448145435863,0.0016786571606254921,-0.0493982033121453,0.15362308208318937,0.1448449213601357,-0.2817021331154275,0.3425951593088034,0.1535748334974595,-0.04466902989734306,-0.14336604777002163,-0.17936948781662512,-0.12725115660733177,0.02460414323556227,0.22999774818953037,-0.08103428957396543,-0.12477101490053613,-0.03098268662656284,0.09407129715176256,0.008929051739341148,-0.11086233867337766,-0.182920484058256,0.08980571776594541,-0.1019073318624501,-0.021381981084584754,-0.13352824593169474,0.17747533036357613,-0.4333686593834771,0.05427924140654873,0.06557023265730362,0.10473728590711953,-0.06882172050806844,-0.03593845563601432,0.27003579463525185,0.09649332400797642,-0.11448646522382577,0.07690010414180323,0.013620198950923135,0.25922280669338654,-0.21942895961206563,0.014635374238708324,-0.312803019477208,-0.011448344035494083,0.11374276822212462,0.004903709537377008,-0.24218305843292076,0.2689585607752234,-0.42273446374716706,0.09518957962414733,-0.10026902048107787,0.31756629465276576,-0.406144282326038,0.11935394954129919,0.1850409454954882,0.397706733808644,0.07277131051367429,-0.20704512986117374,0.3010673408062294,-0.2344712140207455,0.1749850749531898,-0.11333365570742834,-0.07346711793126391,-0.3520227415891699,-0.20185232499235348,-0.10432789117376239,0.423361462865689,0.0842986948540724,0.1683860383576705,-0.29182327204329683,-0.09250491175896174,0.0010004008172849327,-0.025822958943550776,-0.03580430994982825,-0.2910519393246309,-0.08583195282887077,-0.024429245616850604,-0.13802002084079845,-0.038581891854123015,-0.18518956694606323,-0.034105290163537125,0.23884958910519913,0.24009035545783555,0.24579085877664586,-0.006001068191137878,-0.10041599000620027,-0.15538421884976747,0.05746722822760694,0.19720592334350706,0.23144460412655657,-0.057612915641150446,-0.0229047572565293,-0.04901537312212359,-0.22689558002425245,-0.15715071533144662,0.37499900330728486,0.04538303795514197,0.0584035288555647,0.1597688175440844,-0.14677861300121678,0.0863322020898995,-0.23770908743552088,0.21737919712269532,-0.01566901635063177,-0.02885587944819425,0.03916270727916541,0.1983419739592677,0.26175397094926733,0.021664017908347415,0.2142440968201456,-0.27580239240094634,-0.3863960912628072,-0.019958605699697016,-0.19100708765328628,0.12807075311183114,0.09187339537860903,-0.24859699529510887,0.23414287594682331,-0.06580931639538999,-0.05813483735626078,0.03835474844425468,-0.30005143166984094,0.016826799208444945,-0.01142615123968883,-0.256856148378376,-0.13691458797242287,-0.18743790110403147,0.23970881537363248,0.3269047092611775,-0.3257996926671975,0.03976400887271353,-0.12785797875711283,-0.2452496583471573,0.021274084968240996,-0.09634732139561501,-0.2092952463900979,0.06266448919618726,0.21623877731149532,-0.2667450195497977,-0.3364857798766604,-0.08314466033652318,-0.13000147376561355,0.08413805480625554,0.16143195743155972,-0.25400198104928523,-0.02605893957307727,0.030996787928191327,0.20684194067102485,-0.22908604722749434,0.14658300634228144,0.008359869330818028,-0.04327832029341487,-0.16263203676653362,0.08150496267159547,0.10320197330902084,0.11894569825108778,0.31641033080870196,-0.06764712103298187,-0.3589300726470828,-0.1472645838278181,0.058136314648417495,-0.033844627453249634,-0.18897206636363478,-0.23612541760900546,0.35003708632908326,-0.12889045621667955,0.3045315432320243,-0.22130178943931283,-0.11582846963477719,-0.04196925111863009,-0.2418968005188063,0.10093541135194847,0.12585163151464518,-0.08685575245364804,-0.11693024314899866,-0.2737368107454115,-0.237738113353034,-0.13992340828916774,0.18560327787590175,-0.1309857462538571,-0.05608793788712543,-0.06988635658903032,-0.06654736797223892,0.11797244128905197,-0.09715587319092828,0.005214642015510787,-0.3523944264233687,0.01662935714078228,-0.04348987450189401,-0.22124169703858987,-0.31594574577199847,0.02054222473751574,0.12773634179971932,-0.288969448377612,-0.2963473424287258,-0.3395964947506394,-0.29378990141155,0.3033726113853838,-0.17628043171418448,-0.25477212575705316,0.13211452886037747,-0.10768778801052876,0.29422566966113006,-0.14961421621624466,-0.28846994539129656,0.13509997269926882,0.16577636058153505,0.3419254285721775,-0.0615811025847469,0.18477139066433118,0.07987579914090069,-0.2325304658652197,-0.06213232225922877,-0.14500883176836554,0.3086358480419144,-0.15613924138326885,-0.2968739465113445,-0.17931496287962972,-0.10174409452820257,-0.01716434211490393,0.2852086447017932,-0.22794729575889228,0.4301303537939101,-0.09348181078112268,-0.06862215679488022,0.24108170990179184,0.10986673171613275,-0.06720112223758475,-0.3348484853831011,0.0136274877977226,-0.2669224099250998,-0.29184392312588714,0.29420498495639547,0.23725046635114264,-0.27263648654640354,0.41099769542954917,0.21720668899139836,0.18936920323464074,0.23923811465348432,-0.13088244861246567,0.11859200530653434,0.0058003417844120185,0.03445761245374286,-0.04192169194797407,0.22877293820241967,-0.03407366272771504,-0.07846279595481298,0.25853829842239445,-0.16366462961464426,-0.21734575881577892,0.04209758005668033,0.2994664304567233,-0.1275282615310505,-0.1522295589322897,0.13881140015392257,0.03920092564633389,0.36019020563902576,0.07239116103525094,0.1144506247794468,-0.18915582484000915,0.23525294388230777,0.014535081971739052,-0.0022369910267125877,0.046918694934064864,-0.21571876836974807,-0.1897611653069963,-0.13611223368851016,0.13311568053842585,-0.11071475783963462,-0.1718863716895984,-0.10786855814942954,-0.3171738624232556,0.1468742977798893,0.21175382809043908,0.1426837030240439,0.2316152605480296,-0.057895637706017786,0.20391399495475343,-0.07085928079271062,-0.12680891558825,0.09096395278195553,-0.009568544686328666,-0.23967353486826767,-0.18749959575194122,-0.27938843088647425,0.26731985836982863,0.14289449232397594,0.19513501738146793,-0.05423431870404151,0.050913606280979726,0.01309907619182429,0.2577261742511955,0.4662195761164401,0.12253370036657066,0.2606297298333919,-0.09478378766624311,-0.3375230513238934,0.2856598341571302,0.08882595697454476,0.16267742080203368,-0.1689627623603357,0.21061851550711566,0.05836681304739852,0.1981797712588293,0.09802698344255104,0.036281570874612104,-0.24409369501102662,-0.12948229230956837,0.06238143186947197,-0.1645557669642882,-0.10393041380688438,-0.19460325742219722,0.2689809521168569,0.320010845138686,-0.04685741696007818,0.2733668844230247,0.1223075210792522,0.10722035642417031,0.21027879993501747,-0.11642615577236881,-0.05872954473805178,0.22378825609849787,0.049155609826538604,0.04914856796038174,0.2633254780928993,-0.2868920388301942,-0.14226561707653823,0.22463761485232084,-0.05303670799398266,0.13068513043580587,-0.19323361374763312,0.15223742214261884,0.157798675869803,-0.19717514184505816,-0.007092189118704399,-0.18143162200184088,0.028761910942905335,-0.05319852337149763,0.23028178800668508,0.0924722075439632,-0.13266187876004562,-0.27363530771191313,-0.23716930011652315,-0.14345017560311174,0.14367593465277156,-0.02819960161205211,0.10913087112260607,0.23570282310503982,0.012154149912888382,-0.03312258514706887,-0.10252546089551617,-0.08081368311986906,0.04039780874372985,0.018398718828181407,0.11620524951356684,0.19695145526009414,-0.06751568657108747,-0.12489998576711421,-0.04768473353155836,-0.05713469134842125,0.2679890416424535,-0.10585854868142035,0.0020068145242689746,-0.04153361050229766,-0.0009882771757818015,0.045292798338090814,0.08358316228415129,-0.06556956673910441,-0.07864643362403262,-0.03786589734861269,0.12639218284678622,-0.2558647265053586,-0.032746232656036545,-0.0971782254876139,-0.12951510656286272,0.14073934248682374,0.16747054581223741,0.311947704895161,-0.07164878338991405,-0.1882871589442542,0.12897078047364866,-0.04919321670759601,0.3021977511355546,0.27653182553932587,-0.1523730345080975,-0.2837795563634236,-0.219760237431966,0.19208033406231834,-0.22824340014414407,-0.19780352207499344,0.2060960081692158,0.24847421116839272,0.019449952849562854,-0.021053609172838536,-0.2169353798771829,0.07404352066127905,-0.13506362907728223,0.10983929850707708,-0.172270499469749,-0.1311775537947109,0.059471811205782335,-0.2882737897487614,-0.1960434892672735,0.13158907147590437,0.16319340497251025,-0.26565087597434023,-0.24032341386451378,-0.10422943164625809,-0.1496271708198576,0.01308087422176844,-0.13532731790466906,0.030061846805975324,-0.04726202320740089,0.2410519443402374,-0.014541339713685278,-0.30608385002410315,0.14564893017689956,0.053070537442718474,0.26317242156596937,-0.07640291531989338,0.1595263194004821,-0.3764189491464113,-0.1814670669564855,-0.1831274414254364,-0.0717550699077229,0.21388970869921314,0.25265887928543523,0.26424694635502705,0.2659906853768814,0.012997347828961732,-0.012175371657584328,0.0386450281904845,-0.09451244304117248,0.013101243424532071,0.26757521930186823,-0.056429456759601704,0.20706577811201332,0.0218059267605158,-0.22095152740617982,0.004525614606244205,0.03948558315290715,0.046683124141093255,-0.05063304481025906,0.23376682140620814,0.061255879218900924,0.02763323122958172,0.07686491519648674,0.31997809166738117,-0.014262947229010291,0.2835451514551835,0.002239362556283094,0.1006126497857545,-0.1246252655579857,-0.2571626381490746,-0.1864738151489159,-0.13191603318970344,0.1797915721896087,0.3113106150097129,-0.22644081357747187,-0.0237942311390166,-0.15542571302768965,-0.19741736371864352,-0.08408886675514282,0.25446750113314875,0.16679421853058068,0.01989589442484742,-0.301385182724366,-0.013582983546560543,-0.21945263593651335,0.045529950544561784,0.26595968420873933,0.1963738224505781,0.2534775007810666,-0.02364553211385487,-0.033204863932542725,0.1851120260771924,-0.37748674883500855,-0.27764441286250574,-0.10601655403244431,0.023693712402210174,0.08906799746037677,-0.02658827702779946,-0.22725603802726294,0.3044710606690723,0.2838732898381798,-0.2256307892303086,-0.05520877482547609,-0.35180208060837476,0.013900022219093956,-0.18780180507183347,0.3482479286747733,0.020035323996893502,-0.00863350293080449,0.06683305651116594,-0.23562188177130305,0.3242500664630328,0.11391695502530493,-0.030569373075085855,-0.013190328670760247,0.1361593160098054,0.3235770529285721,-0.07013231661768424,0.06777645189059205,0.14420704074114607,0.186477942971421,0.22264639762354085,-0.012673691431062888,0.10420986478677888,-0.02159381672883361,-0.2927064052939591,-0.048493104610109763,-0.09003379108200193,-0.13479011337702132,0.13647399520751533,-0.3561114094072085,-0.2682164120629615,-0.12217040359002006,-0.2306037616670507,-0.11021576492600632,-0.2770666471099696,-0.0013511263686186402,0.04222845059966433,-0.010538614488967198,-0.18400279410554568,0.09513923209588199,-0.06954149866644964,-0.37098358556300165,0.04271160370160454,0.27869989985670773,0.27736851052234873,-0.24122458106232922,-0.14118496794057422,0.23967563861033933,0.2976411708328736,-0.2715507814334052,0.019717716687433762,-0.12405065219206027,0.23720773192092118,0.027332563240133403,0.3628324501082315,0.04511625325341652,-0.12368545372889223,0.04942815304979904,0.09584819263084539,0.018647987954791,0.12316339573873948,-0.028527552517176095,-0.3978407660765021,0.21434355310307324,0.17854159901474695,-0.0001942501837503205,0.13133528409043324,-0.07158663110033628,-0.00958450529208978,-0.08435002674479035,-0.26568181280122527,-0.42166735927694954,0.0836954150219805,0.043359755379223916,0.4060339368977123,-0.03864644986378361,-0.19904905423537103,0.13702192206127703,0.14522222951108288,0.04768022676393477,0.20216082742628305,0.14298286479584535,-0.2575327061684961,-0.1924586882698021,0.20180433115449514,0.065696162761679,-0.3614971385504614,0.07949059023539012,-0.10888844561522479,-0.10419481371284037,-0.21703161495181136,0.17764970552150525,0.04458433981580143,0.08981134769701725,-0.034816916087071326,0.14947936132481032,-0.22288273966147223,-0.2161642483921692,-0.1921381697388496,-0.11103625459290416,0.19011163027779382,0.003937098198110578,-0.10542466342487497,0.15038860626227746,-0.3803369814176114,0.10395043268601417,0.3529802289987019,0.13949926429686493,-0.1388455530193028,-0.3221798451657033,0.13635307324772922,0.15762772893814542,-0.10061058629524304,-0.08453094295907534,-0.1419282907504198,-0.09221023325291829,0.00021824253679551472,-0.12351567718087086,0.037106811938531395,0.44244135724701267,-0.07590202125739619,-0.15970547037749472,-0.21406197514012837,0.13072395058225308,0.3398211246813775,-0.10863745854196426,0.2072123220738899,0.24881414587518474,0.09389684072256019,-0.3173683729269065,0.30014524652527985,-0.08050779918053022,-0.005886676024188575,-0.2778882455272026,0.18583574872030229,-0.16567152014718717,-0.35727307109356365,-0.030295929037844036,-0.32819002733286684,-0.03446725509549951,0.37579796686581207,-0.27879383513196987,-0.062551091006496,0.13617593254966157,-0.1947865920267562,0.37974406433104296,-0.43488078978976435,-0.11327795976366567,0.01809717296490981,0.029874176501447358,-0.1392848793797459,0.18017485250956064,0.031284523325566894,-0.1806598741931622,-0.08947170962846727,0.1756291335075213,-0.04242415924154045,0.024940451747336122,-0.002602294586236776,0.006526416544662662,-0.358695950102503,0.30135458012247435,0.11930316596050908,-0.544631648298706,0.304483996495782,-0.11811722540660546,0.1760079369542773,0.20901231811726234,-0.13534962787361665,-0.09135839485080367,-0.28030535929903677,-0.3162811198127125,0.10869145341919954,0.17503707220870948,-0.2601427833407244,});
}


inline float LR::dot (const vector<float> & vec1, const vector<float> & vec2) {

#ifndef NO_NEON
    int len = vec1.size();
    const float * p_vec1 = &vec1[0];
    const float * p_vec2 = &vec2[0];


    float sum=0;
    float32x4_t sum_vec = vdupq_n_f32(0),left_vec,right_vec;
    for(int i = 0; i < len; i += 4)
    {
        left_vec = vld1q_f32(p_vec1 + i);
        right_vec = vld1q_f32(p_vec2 + i);
        sum_vec = vmlaq_f32(sum_vec, left_vec, right_vec);
    }

    float32x2_t r = vadd_f32(vget_high_f32(sum_vec), vget_low_f32(sum_vec));
    sum += vget_lane_f32(vpadd_f32(r,r),0);
#else
    float sum = 0.0;
    for (int i = 0; i < vec1.size(); i++) {
        sum += vec1[i] * vec2[i];
    }
#endif
    return sum;
}


inline float LR::sigmoidCalc(const float wxb) {
    float expv = exp(-1 * wxb);
    float expvInv = 1 / (1 + expv);
    return expvInv;
}
inline float LR::lossCal(const vector<float> & weight) {
    float lossV = 0.0L;
    int i;

    for (i = 0; i < train_data_.size(); i++) {
        lossV -= train_data_[i].label * log(sigmoidCalc(dot(train_data_[i].features, weight)));
        lossV -= (1 - train_data_[i].label) * log(1 - sigmoidCalc(dot(train_data_[i].features, weight)));
    }
    lossV /= train_data_.size();
    return lossV;
}

inline float LR::gradientSlope(const vector<Data> & dataSet, int index, const vector<float> & sigmoidVec) {
    float gsV = 0.0L;
    float sigv, label;
    for (int i = 0; i < dataSet.size(); i++) {
        sigv = sigmoidVec[i];
        label = dataSet[i].label;
        gsV += (label - sigv) * (dataSet[i].features[index]);
    }

    gsV = gsV / dataSet.size();
    return gsV;
}

inline vector<Data> LR::LoadData(const string & filename, bool last_label_exist ,int read_line_num)
{

    FILE * fp = NULL;
    char * line, * record;
    char buffer[20000];


    if ((fp = fopen(filename.c_str(), "rb")) == NULL) {
        printf("file [%s] doesnnot exist \n", filename.c_str());
        exit(1);
    }

    vector<Data> data_set;
    int cnt = 0;
    while ((line = fgets(buffer, sizeof(buffer), fp)) != NULL) {

        if (last_label_exist && cnt >= read_line_num) break;
        cnt++;

        vector<float> feature(features_num + (last_label_exist? 1: 0), 0.0);

        int f_cnt = 0;
        record = strtok(line, ",");
        while (record != NULL && f_cnt < features_start + feature.size()) {
//            printf("%s ", record);
            if (f_cnt >= features_start) {
                feature[f_cnt - features_start] = atof(record);
            }
            f_cnt++;
            record = strtok(NULL, ","); // 每个特征值

        }

        if (last_label_exist) {
            int ftf = (int) feature.back();
            feature.pop_back();
            data_set.push_back(Data(feature, ftf));
        } else {
            data_set.push_back(Data(feature, 0));
        }
    }
    fclose(fp);
    fp = NULL;


    return data_set;
}


inline void LR::StorePredict()
{
    string line;
    int i = 0;

    ofstream fout(predict_file_.c_str());
    if (!fout.is_open()) {
        printf("打开预测结果文件失败");
        exit(1);
    }
    for (i = 0; i < predict_vec.size(); i++) {
        fout << predict_vec[i] << endl;
    }
    fout.close();
}

bool loadAnswerData(string awFile, vector<int> & awVec) {
    ifstream infile(awFile.c_str());
    if (!infile) {
        cout << "打开答案文件失败" << endl;
        exit(0);
    }

    while (infile) {
        string line;
        int aw;
        getline(infile, line);
        if (line.size() > 0) {
            stringstream sin(line);
            sin >> aw;
            awVec.push_back(aw);
        }
    }

    infile.close();
    return true;
}



void Test (string answerFile, string predictFile) {
    vector<int> answerVec;
    vector<int> predictVec;
    int correctCount;
    float accurate;

    cout << "ready to load answer data" << endl;
    loadAnswerData(answerFile, answerVec);
    loadAnswerData(predictFile, predictVec);

    cout << "test data set size is " << predictVec.size() << endl;
    correctCount = 0;
    for (int j = 0; j < predictVec.size(); j++) {
        if (j < answerVec.size()) {
            if (answerVec[j] == predictVec[j]) {
                correctCount++;
            }
        } else {
            cout << "answer size less than the real predicted value" << endl;
        }
    }

    accurate = ((float)correctCount) / answerVec.size();
    cout << "the prediction accuracy is " << accurate << endl;
}

int main(int argc, char *argv[])
{

#ifdef TEST
    clock_t start_time = clock() ;
#endif


#ifdef TEST // 线下测试用的数据路径
    string train_file = "../data/train_data.txt";
    string test_file = "../data/test_data.txt";
    string predict_file = "../data/result.txt";
    string answer_file = "../data/answer.txt";
#else // 提交到线上，官方要求的数据路径
    string train_file = "/data/train_data.txt";
    string test_file = "/data/test_data.txt";
    string predict_file = "/projects/student/result.txt";
    string answer_file = "/projects/student/answer.txt";
#endif

    LR logist(train_file, test_file, predict_file);


    logist.train();

    logist.predict();


#ifdef TEST
    clock_t end_time = clock();
    printf("总耗时（s）: %f \n", (double) ((double) (end_time - start_time) / CLOCKS_PER_SEC) / CLOCKS_PER_SEC);
#endif

#ifdef TEST
    Test(answer_file, predict_file);
#endif

    return 0;
}
