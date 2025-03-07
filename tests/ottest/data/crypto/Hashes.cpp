// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ottest/data/crypto/Hashes.hpp"  // IWYU pragma: associated

#include <initializer_list>

#include "opentxs/util/Container.hpp"

namespace ottest
{
using namespace std::literals;

auto Argon2i() noexcept -> std::span<const ArgonVector>
{
    // https://github.com/P-H-C/phc-winner-argon2/blob/master/src/test.c
    // Modified salt length and iteration count to correspond to libsodium
    // requirements
    // Checked against https://argon2.online/
    static const auto data = ot::Vector<ArgonVector>{
        {
            4,
            (1 << 16) << 10,  // 65536 KiB
            1,
            "password"sv,
            "somesaltsomesalt"sv,
            "eba00eb46e3219304995c15da7a4d5b90a79b2712c4776c1038413dd07b2f33a"sv,
        },
        {
            4,
            (1 << 8) << 10,  // 256 KiB
            1,
            "password"sv,
            "somesaltsomesalt"sv,
            "797ebc52b990ebdde9adc36a11330f48f292c770ad4fb4e77e5883964d1f7ebb"sv,
        },
        {
            4,
            (1 << 8) << 10,  // 256 KiB
            2,
            "password"sv,
            "somesaltsomesalt"sv,
            "26e3e3e4e1d020f2d3bac77177b498752ed847a606da04b5594dc4e4e4a003ca"sv,
        },
        {
            3,
            (1 << 16) << 10,  // 65536 KiB
            1,
            "password"sv,
            "somesaltsomesalt"sv,
            "7d1b1163d3c0b791fea802ae5d1ccbd3fe896c54a1b0277ad96e5a1f311293f7"sv,
        },
        {
            6,
            (1 << 16) << 10,  // 65536 KiB
            1,
            "password"sv,
            "somesaltsomesalt"sv,
            "bc4b50127a30b18377f81d26754a8ad3652554da170df1f5ff9576bcb76a6cca"sv,
        },
        {
            4,
            (1 << 16) << 10,  // 65536 KiB
            1,
            "differentpassword"sv,
            "somesaltsomesalt"sv,
            "58267b9e980abacb368cc1512cc959607ee18de7dce74c0e8bb991c1ead63f41"sv,
        },
        {
            4,
            (1 << 16) << 10,  // 65536 KiB
            1,
            "password"sv,
            "diffsaltdiffsalt"sv,
            "3c71a874e9b82570c3d584e4379c06bf5e7f4af5357b62889b0915a0601ef2bd"sv,
        },
    };

    return data;
}

auto Argon2id() noexcept -> std::span<const ArgonVector>
{
    // https://github.com/P-H-C/phc-winner-argon2/blob/master/src/test.c
    // Modified salt length to correspond to libsodium requirements
    // Checked against https://argon2.online/
    static const auto data = ot::Vector<ArgonVector>{
        {
            2,
            (1 << 16) << 10,  // 65536 KiB
            1,
            "password"sv,
            "somesaltsomesalt"sv,
            "fc33b78139231d34b71626bd6245c1d72efa190ad605c3d8166a72adcedfa2c2"sv,
        },
        {
            2,
            (1 << 8) << 10,  // 256 KiB
            1,
            "password"sv,
            "somesaltsomesalt"sv,
            "cab746b4621993fdc91ec50787980b414a90a692f0bc68dfe19f9c25b3cba9ec"sv,
        },
        {
            2,
            (1 << 8) << 10,  // 256 KiB
            2,
            "password"sv,
            "somesaltsomesalt"sv,
            "c112d2ee6b9d514413f806243187952186e8f19cacab80f20a823d549f111d2c"sv,
        },
        {
            1,
            (1 << 16) << 10,  // 65536 KiB
            1,
            "password"sv,
            "somesaltsomesalt"sv,
            "ec2b46acb6f8aec6804bf8df88feeca36a4412df3bea8d2cc99c08a9e8977a72"sv,
        },
        {
            4,
            (1 << 16) << 10,  // 65536 KiB
            1,
            "password"sv,
            "somesaltsomesalt"sv,
            "34a6f651a0ab14f3b15f86115f0ae5532e1029365d8c218f47d24f66dab2688f"sv,
        },
        {
            2,
            (1 << 16) << 10,  // 65536 KiB
            1,
            "differentpassword"sv,
            "somesaltsomesalt"sv,
            "b2a645d33be125cd75f64b157793464041c0fd196b848a9f78508e1a0e5ce4c0"sv,
        },
        {
            2,
            (1 << 16) << 10,  // 65536 KiB
            1,
            "password"sv,
            "diffsaltdiffsalt"sv,
            "c22e7f0935f83ed7a0163bfd4f09a2014b94c9aa19dc681e7e781170dcfd1659"sv,
        },
    };

    return data;
}

auto EthHashes() noexcept -> std::span<const EthereumVector>
{
    // Generated using https://www.rfctools.com/ethereum-address-test-tool/
    static constexpr auto data = std::initializer_list<EthereumVector>{
        {
            "1247328147577841EDAB7CE43A31D49C722152C28229D84C28722594D16B8A29"sv,
            "1be26e236e77da12843aa641c806be6cde6977dec790c86bfe84ec3fa4535826f6288d62fd77fdd61061052f82959211059547bfdb44cab932f682aff4a89918"sv,
            "997d2b9fb17bed5422bcd616e14a6a8bf2f28a1a8fda205232084c74faa19531"sv,
            "e14a6a8bf2f28a1a8fda205232084c74faa19531"sv,
            "7b8f26e84c37225b697d9719545e03e7546ecf69cce647b9c1d0ea68810d8721"sv,
            "e14A6a8Bf2f28a1A8FdA205232084c74faa19531"sv,
        },
        {
            "E9613EB5DE38D4B8763813913C8493E20D9BB725744A89132B777897AD2E6110"sv,
            "ec55462bb46842e5c9aa3bc49f15bcb714a128d8dbffffc4a60106e385bc3bc466ccaa3b0a767afe2c66a66221449cc7e7482944e13a17a755a4dde0801e03fc"sv,
            "697c4ee0537b3f74290fb32faa9c7e21d31712cc8a23314ce550052c96b62eea"sv,
            "aa9c7e21d31712cc8a23314ce550052c96b62eea"sv,
            "a87372e8d9b4304edcb623931884843ac28cc9c941f0a5c357d18a87bd2a0d47"sv,
            "AA9c7e21D31712cC8A23314ce550052C96B62EEA"sv,
        },
        {
            "622CCEC9DC0E38CCEC65099CEE973BD0DB209263C39DCEA31410B0E9952D0A1E"sv,
            "3493c1baea56ea6979ae0ba3eebff66e72433016872b49895dcb6b3ca192a2019315c8f42d8f6108b2166bc1cac5cc7a02cd54d5dfe6409d1c0735b93fa86f0d"sv,
            "da3b5b385dba4ad8a06bdf25191fd4aa32edd0a6a199079b3c32c979dbc72980"sv,
            "191fd4aa32edd0a6a199079b3c32c979dbc72980"sv,
            "140b32f3ab1a9f3ecc8b75e4738201f6042e1a73a25219fd243367e058cefe16"sv,
            "191Fd4Aa32eDD0a6A199079b3c32c979dbc72980"sv,
        },
        {
            "DD665AE694722A3813C42155B888DD367047EA121138542D94EDD00293CC3271"sv,
            "d307e35ef9ef4c5efd66d9039f399994b2c909ac9cf026934d678019d7db2d9ecd1c163dfad20504d790ddad3ec7c19031169e9e4d40d89cf540d92d1320d7bc"sv,
            "fdcc5b29a0408b069d4ccdde10330c7f31d50ef2193802d8664f7285527a260f"sv,
            "10330c7f31d50ef2193802d8664f7285527a260f"sv,
            "bb1b734623b9e391cc5f4759a0d84a3c04f671616721cf356c7d0fce75ed18da"sv,
            "10330c7f31D50eF2193802d8664F7285527a260f"sv,
        },
        {
            "BC149CAA45A9E84A2E29AD9010092C3082620ACA378282B8E741B68D5CCEC017"sv,
            "714fe34e719331594d980385a62b8b509d3112e1090184c9b02d5158b832284335c53001b70bc51abfb17e467316b196cd5f261ed5595c8afb486a442f8ffa74"sv,
            "b96d34881ada92c2edd09df2b4bb730813fe4a3be56c6665f836a73cefba6eba"sv,
            "b4bb730813fe4a3be56c6665f836a73cefba6eba"sv,
            "74d9c07348b3b8f8a6150ce8e2a37655c20e4a6ebff73da8e375f28febb5afb1"sv,
            "b4BB730813Fe4A3BE56c6665F836a73cEfbA6EbA"sv,
        },
    };

    return data;
}

auto Keccak256() noexcept -> std::span<const KeccakVector>
{
    // https://github.com/mighty-gerbils/gerbil-crypto/blob/master/t/keccak-test.ss
    static constexpr auto data = std::initializer_list<KeccakVector>{
        {""sv,
         "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470"sv},
        {"arsttsra"sv,
         "b1f354b28ff28452d6b9d61f41061bbe82beb1fc98f33364a8058d1a5d164d05"sv},
        {"arst"sv,
         "c3305bc9de1244e48050962ced50b75934c37006e99e8b7a62213e945c3dfcd7"sv},
        {"abc"sv,
         "4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45"sv},
        {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"sv,
         "45d3b367a6904e6e8d502ee04999a7c27647f91fa845d456525fd352ae3d7371"sv},
        {"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"sv,
         "f519747ed599024f3882238e5ab43960132572b7345fbeb9a90769dafd21ad67"sv},
        {"this is a test"sv,
         "9fd09c38c2a5ae0a0bcd617872b735e37909ccc05c956460be7d3d03d881a0dc"sv},
    };

    return data;
}

auto Murmur() noexcept -> std::span<const MurmurVector>
{
    // https://stackoverflow.com/a/31929528
    static const auto data = ot::Vector<MurmurVector>{
        {""sv, 0, 0},
        {""sv, 1, 1364076727},
        {""sv, 4294967295, 2180083513},
        {"0xffffffff"sv, 0, 1982413648},
        {"0x21436587"sv, 0, 4116402539},
        {"0x21436587"sv, 1350757870, 593689054},
        {"0x214365"sv, 0, 2118813236},
        {"0x2143"sv, 0, 2700587130},
        {"0x21"sv, 0, 1919294708},
        {"0x00000000"sv, 0, 593689054},
        {"0x000000"sv, 0, 2247144487},
        {"0x0000"sv, 0, 821347078},
        {"0x00"sv, 0, 1364076727},
    };

    return data;
}

auto NistBasic() noexcept -> std::span<const NistHashVector>
{
    // https://www.di-mgt.com.au/sha_testvectors.html
    static const auto data = ot::Vector<NistHashVector>{
        {
            "abc"sv,
            "a9993e364706816aba3e25717850c26c9cd0d89d"sv,
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"sv,
            "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f"sv,
        },
        {
            ""sv,
            "da39a3ee5e6b4b0d3255bfef95601890afd80709"sv,
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"sv,
            "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"sv,
        },
        {
            "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"sv,
            "84983e441c3bd26ebaae4aa1f95129e5e54670f1"sv,
            "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"sv,
            "204a8fc6dda82f0a0ced7beb8e08a41657c16ef468b228a8279be331a703c33596fd15c13b1b07f9aa1d3bea57789ca031ad85c7a71dd70354ec631238ca3445"sv,
        },
        {
            "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"sv,
            "a49b2446a02c645bf419f995b67091253a04a259"sv,
            "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1"sv,
            "8e959b75dae313da8cf4f72814fc143f8f7779c6eb9f7fa17299aeadb6889018501d289e4900f7e4331b99dec4b5433ac7d329eeb6dd26545e96e55b874be909"sv,
        },
    };

    return data;
}

auto NistMillion() noexcept -> const NistHashVector&
{
    static const auto data = NistHashVector{
        "a"sv,
        "34aa973cd4c4daa4f61eeb2bdbad27316534016f"sv,
        "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"sv,
        "e718483d0ce769644e2e42c7bc15b4638e1f98b13b2044285632a803afa973ebde0ff244877ea60a4cb0432ce577c31beb009c5c2c49aa2e4eadb217ad8cc09b"sv,
    };

    return data;
}

auto NistGigabyte() noexcept -> const NistHashVector&
{
    static const auto data = NistHashVector{
        "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno"sv,
        "7789f0c9ef7bfc40d93311143dfbe69e2017f592"sv,
        "50e72a0e26442fe2552dc3938ac58658228c0cbfb1d2ca872ae435266fcd055e"sv,
        "b47c933421ea2db149ad6e10fce6c7f93d0752380180ffd7f4629a712134831d77be6091b819ed352c2967a2e2d4fa5050723c9630691f1a05a7281dbe6c1086"sv,
    };

    return data;
}

auto PBKDF_sha1() noexcept -> std::span<const PBKDFVector>
{
    // https://tools.ietf.org/html/rfc6070
    static const auto data = ot::Vector<PBKDFVector>{
        {
            "password"sv,
            "salt"sv,
            1,
            20,
            "0x0c60c80f961f0e71f3a9b524af6012062fe037a6"sv,
        },
        {
            "password"sv,
            "salt"sv,
            2,
            20,
            "0xea6c014dc72d6f8ccd1ed92ace1d41f0d8de8957"sv,
        },
        {
            "password"sv,
            "salt"sv,
            4096,
            20,
            "0x4b007901b765489abead49d926f721d065a429c1"sv,
        },
        {
            "password"sv,
            "salt"sv,
            16777216,
            20,
            "0xeefe3d61cd4da4e4e9945b3d6ba2158c2634e984"sv,
        },
        {
            "passwordPASSWORDpassword"sv,
            "saltSALTsaltSALTsaltSALTsaltSALTsalt"sv,
            4096,
            25,
            "0x3d2eec4fe41c849b80c8d83662c0e44a8b291a964cf2f07038"sv,
        },
    };

    return data;
}

auto PBKDF_sha256() noexcept -> std::span<const PBKDFVector>
{
    // https://github.com/Anti-weakpasswords/PBKDF2-Test-Vectors/releases
    static const auto data = ot::Vector<PBKDFVector>{
        {
            "password"sv,
            "salt"sv,
            1,
            32,
            "120FB6CFFCF8B32C43E7225256C4F837A86548C92CCC35480805987CB70BE17B"sv,
        },
        {
            "password"sv,
            "salt"sv,
            2,
            32,
            "AE4D0C95AF6B46D32D0ADFF928F06DD02A303F8EF3C251DFD6E2D85A95474C43"sv,
        },
        {
            "password"sv,
            "salt"sv,
            4096,
            32,
            "C5E478D59288C841AA530DB6845C4C8D962893A001CE4E11A4963873AA98134A"sv,
        },
        {
            "passwordPASSWORDpassword"sv,
            "saltSALTsaltSALTsaltSALTsaltSALTsalt"sv,
            4096,
            40,
            "348C89DBCBD32B2F32D814B8116E84CF2B17347EBC1800181C4E2A1FB8DD53E1C635518C7DAC47E9"sv,
        },
        {
            "password"sv,
            "salt"sv,
            16777216,
            32,
            "CF81C66FE8CFC04D1F31ECB65DAB4089F7F179E89B3B0BCB17AD10E3AC6EBA46"sv,
        },
    };

    return data;
}

auto PBKDF_sha512() noexcept -> std::span<const PBKDFVector>
{
    // https://github.com/Anti-weakpasswords/PBKDF2-Test-Vectors/releases
    static const auto data = ot::Vector<PBKDFVector>{
        {
            "password"sv,
            "salt"sv,
            1,
            64,
            "867F70CF1ADE02CFF3752599A3A53DC4AF34C7A669815AE5D513554E1C8CF252C02D470A285A0501BAD999BFE943C08F050235D7D68B1DA55E63F73B60A57FCE"sv,
        },
        {
            "password"sv,
            "salt"sv,
            2,
            64,
            "E1D9C16AA681708A45F5C7C4E215CEB66E011A2E9F0040713F18AEFDB866D53CF76CAB2868A39B9F7840EDCE4FEF5A82BE67335C77A6068E04112754F27CCF4E"sv,
        },
        {
            "password"sv,
            "salt"sv,
            4096,
            64,
            "D197B1B33DB0143E018B12F3D1D1479E6CDEBDCC97C5C0F87F6902E072F457B5143F30602641B3D55CD335988CB36B84376060ECD532E039B742A239434AF2D5"sv,
        },
        {
            "passwordPASSWORDpassword"sv,
            "saltSALTsaltSALTsaltSALTsaltSALTsalt"sv,
            4096,
            64,
            "8C0511F4C6E597C6AC6315D8F0362E225F3C501495BA23B868C005174DC4EE71115B59F9E60CD9532FA33E0F75AEFE30225C583A186CD82BD4DAEA9724A3D3B8"sv,
        },
    };

    return data;
}

auto rfc4231() noexcept -> std::span<const HMACVector>
{
    // https://tools.ietf.org/html/rfc4231
    static const auto data = ot::Vector<HMACVector>{
        {
            "0x0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b"sv,
            "0x4869205468657265"sv,
            "0xb0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7"sv,
            "0x87aa7cdea5ef619d4ff0b4241a1d6cb02379f4e2ce4ec2787ad0b30545e17cdedaa833b7d6b8a702038b274eaea3f4e4be9d914eeb61f1702e696c203a126854"sv,
        },
        {
            "0x4a656665"sv,
            "0x7768617420646f2079612077616e7420666f72206e6f7468696e673f"sv,
            "0x5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843"sv,
            "0x164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea2505549758bf75c05a994a6d034f65f8f0e6fdcaeab1a34d4a6b4b636e070a38bce737"sv,
        },
        {
            "0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"sv,
            "0xdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"sv,
            "0x773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe"sv,
            "0xfa73b0089d56a284efb0f0756c890be9b1b5dbdd8ee81a3655f83e33b2279d39bf3e848279a722c806b485a47e67c807b946a337bee8942674278859e13292fb"sv,
        },
        {
            "0x0102030405060708090a0b0c0d0e0f10111213141516171819"sv,
            "0xcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd"sv,
            "0x82558a389a443c0ea4cc819899f2083a85f0faa3e578f8077a2e3ff46729665b"sv,
            "0xb0ba465637458c6990e5a8c5f61d4af7e576d97ff94b872de76f8050361ee3dba91ca5c11aa25eb4d679275cc5788063a5f19741120c4f2de2adebeb10a298dd"sv,
        },
        {
            "0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"sv,
            "0x54657374205573696e67204c6172676572205468616e20426c6f636b2d53697a65204b6579202d2048617368204b6579204669727374"sv,
            "0x60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54"sv,
            "0x80b24263c7c1a3ebb71493c1dd7be8b49b46d1f41b4aeec1121b013783f8f3526b56d037e05f2598bd0fd2215d6a1e5295e64f73f63f0aec8b915a985d786598"sv,
        },
    };

    return data;
}

auto rfc7914() noexcept -> std::span<const ScryptVector>
{
    // https://tools.ietf.org/html/rfc7914
    static const auto data = ot::Vector<ScryptVector>{
        {
            ""sv,
            ""sv,
            16,
            1,
            1,
            64,
            "77d6576238657b203b19ca42c18a0497f16b4844e3074ae8dfdffa3fede21442fcd0069ded0948f8326a753a0fc81f17e8d3e0fb2e0d3628cf35e20c38d18906"sv,
        },
        {
            "password"sv,
            "NaCl"sv,
            1024,
            8,
            16,
            64,
            "fdbabe1c9d3472007856e7190d01e9fe7c6ad7cbc8237830e77376634b3731622eaf30d92e22a3886ff109279d9830dac727afb94a83ee6d8360cbdfa2cc0640"sv,
        },
        {
            "pleaseletmein"sv,
            "SodiumChloride"sv,
            16384,
            8,
            1,
            64,
            "7023bdcb3afd7348461c06cd81fd38ebfda8fbba904f8e3ea9b543f6545da1f2d5432955613f0fcf62d49705242a9af9e61e85dc0d651e40dfcf017b45575887"sv,
        },
        {
            "pleaseletmein"sv,
            "SodiumChloride"sv,
            1048576,
            8,
            1,
            64,
            "2101cb9b6a511aaeaddbbe09cf70f881ec568d574a2ffd4dabe5ee9820adaa478e56fd8f4ba5d09ffa1c6d927c40f4c337304049e8a952fbcbf45c6fa77a41a4"sv,
        },
    };

    return data;
}

auto ScryptLitecoin() noexcept -> std::span<const ScryptVector>
{
    // https://www.litecoin.info/index.php/Block_hashing_algorithm
    static const auto data = ot::Vector<ScryptVector>{
        {
            "01000000ae178934851bfa0e83ccb6a3fc4bfddff3641e104b6c4680c31509074e699be2bd672d8d2199ef37a59678f92443083e3b85edef8b45c71759371f823bab59a97126614f44d5001d45920180"sv,
            ""sv,
            1024,
            1,
            1,
            32,
            "01796dae1f78a72dfb09356db6f027cd884ba0201e6365b72aa54b3b00000000"sv,
        },
    };

    return data;
}

auto X11Vectors() noexcept -> std::span<const X11Vector>
{
    // https://github.com/dashpay/x11-hash-js/blob/master/test/test.js
    static const auto data = ot::Vector<X11Vector>{
        {
            ""sv,
            "51b572209083576ea221c27e62b4e22063257571ccb6cc3dc3cd17eb67584eba"sv,
        },
        {
            "The quick brown fox jumps over the lazy dog"sv,
            "534536a4e4f16b32447f02f77200449dc2f23b532e3d9878fe111c9de666bc5c"sv,
        },
        {
            "DASH"sv,
            "fe809ebca8753d907f6ad32cdcf8e5c4e090d7bece5df35b2147e10b88c12d26"sv,
        },
        {
            "Take this kiss upon the brow! And, in parting from you now, Thus much let me avow-- You are not wrong, who deem That my days have been a dream; Yet if hope has flown away In a night, or in a day, In a vision, or in none, Is it therefore the less gone? All that we see or seem Is but a dream within a dream. I stand amid the roar Of a surf-tormented shore, And I hold within my hand Grains of the golden sand-- How few! yet how they creep Through my fingers to the deep, While I weep--while I weep! O God! can I not grasp Them with a tighter clasp? O God! can I not save One from the pitiless wave? Is all that we see or seem But a dream within a dream?"sv,
            "5c0996b9d49dbe84e29f1b818c1fa9e73549f894a71b8a258964b8f0ecf3c866"sv,
        },
    };

    return data;
}
}  // namespace ottest
