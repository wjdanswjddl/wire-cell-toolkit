#include "WireCellAux/TensorDMdataset.h"
#include "WireCellAux/TensorDMcommon.h"
#include "WireCellAux/SimpleTensor.h"
#include "WireCellAux/TensorTools.h"
#include "WireCellAux/SimpleTensor.h"

#include "WireCellUtil/doctest.h"
#include "WireCellUtil/Logging.h"


#include <complex>
#include <vector>
#include <memory>
#include <iostream>


using real_t = float;
using RV = std::vector<real_t>;
using complex_t = std::complex<real_t>;
using CV = std::vector<complex_t>;

// test fodder
const RV real_vector{0,1,2,3,4,5};
const RV real_vector_cw{0,3,1,4,2,5};
const CV complex_vector{{0,0},{1,1},{2,2},{3,3},{4,4},{5,5}};
const WireCell::ITensor::shape_t shape{2,3};

using namespace WireCell;
using namespace WireCell::Aux;


TEST_CASE("tensordm dataset is type")
{
    auto rt = std::make_shared<SimpleTensor>(shape, real_vector.data());
    CHECK(Aux::is_type<real_t>(rt));
    CHECK(!Aux::is_type<complex_t>(rt));
}


TEST_CASE("tensordm dataset row major")
{
    // ST actually does not let us do anything but C-order/row-major
    auto rm = std::make_shared<SimpleTensor>(shape, real_vector.data());
    CHECK(Aux::is_row_major(rm));
}

template<typename VectorType>
void assert_equal(const VectorType& v1, const VectorType& v2)
{
    CHECK(v1.size() == v2.size());
    for (size_t ind=0; ind<v1.size(); ++ind) {
        CHECK(v1[ind] == v2[ind]);
    }
}

// asvec 1) match type, 2) type mismatch
TEST_CASE("tensordm dataset asvec")
{
    auto rt = std::make_shared<SimpleTensor>(shape, real_vector.data());
    auto ct = std::make_shared<SimpleTensor>(shape, complex_vector.data());
    auto got_rt = Aux::asvec<real_t>(rt);
    auto got_ct = Aux::asvec<complex_t>(ct);
    assert_equal(real_vector, got_rt);
    assert_equal(complex_vector, got_ct);
    CHECK_THROWS_AS(Aux::asvec<complex_t>(rt), ValueError);
}

TEST_CASE("tensordm dataset asarray")
{
    // as array 2x2: (1d,2d) x (rw,cw)

    // make mutable copy to test that TT returns a copy
    RV my_vec(real_vector.begin(), real_vector.end());

    // test 2d
    auto rt = std::make_shared<SimpleTensor>(shape, my_vec.data());
    auto ra = Aux::asarray<real_t>(rt);
    auto shape = rt->shape();
    for (size_t irow = 0; irow < shape[0]; ++irow) {
        for (size_t icol = 0; icol < shape[1]; ++icol) {        
            CHECK(ra(irow, icol) == my_vec[irow*shape[1] + icol]);
        }
    }

    // test 1d
    const WireCell::ITensor::shape_t shape1d{6,};
    auto rt1d = std::make_shared<SimpleTensor>(shape1d, my_vec.data());
    auto ra1d = Aux::asarray<real_t>(rt1d);
    for (size_t ind = 0; ind < shape[0]; ++ind) {        
        CHECK(ra1d(ind) == my_vec[ind]);
    }

    // Assure the internal use of Eigen::Map leads to a copy on return
    my_vec[0] = 42;
    CHECK(ra(0,0) == 0);
    CHECK(ra1d(0) == 0);
}

template<typename ElementType>
void test_array_roundtrip()
{
    using namespace WireCell::PointCloud;
    using namespace WireCell::Aux;

    std::vector<ElementType> v{1,2,3};
    Array::shape_t shape = {3,};
    Array arr(v, shape, true);
    arr.metadata()["foo"] = "bar";

    CHECK(arr.size_major() == 3);
    CHECK(arr.is_type<ElementType>());

    CHECK(arr.metadata()["foo"].asString() == "bar");
    auto itp = TensorDM::as_tensor(arr, "test");
    CHECK(arr.bytes().data() != itp->data());
    CHECK(arr.shape() == itp->shape());
    for (size_t ind=0; ind<v.size(); ++ind) {
        ElementType got = ((ElementType*)itp->data())[ind];
        CHECK(v[ind] == got);
    }
    CHECK(! itp->metadata()["foo"].isNull());
    CHECK(itp->metadata()["foo"].isString());
    CHECK(itp->metadata()["foo"].asString() == "bar");

    Array arr2 = TensorDM::as_array(itp, true);
    CHECK(arr2.shape() == arr.shape());
    CHECK(arr.size_major() == 3);
    CHECK(arr.is_type<ElementType>());
    CHECK(arr2.element<ElementType>(0) == arr.element<ElementType>(0));
    CHECK(arr2.metadata()["foo"].asString() == "bar");
    CHECK(arr2.metadata()["name"].isNull());

    for (size_t ind=0; ind<v.size(); ++ind) {
        CHECK(v[ind] == arr2.element<ElementType>(ind));
    }

    arr2.assure_mutable();      // comment out and see asserts fail.
    itp = nullptr;              // trigger deletion

    CHECK(arr2.shape() == arr.shape());
    CHECK(arr.size_major() == 3);
    CHECK(arr.is_type<ElementType>());
    CHECK(arr2.element<ElementType>(0) == arr.element<ElementType>(0));
    CHECK(arr2.metadata()["foo"].asString() == "bar");
    CHECK(arr2.metadata()["name"].isNull());

    for (size_t ind=0; ind<v.size(); ++ind) {
        CHECK(v[ind] == arr2.element<ElementType>(ind));
    }

}

TEST_CASE("tensordm dataset array roundtrip")
{
    test_array_roundtrip<int8_t>();
    test_array_roundtrip<int16_t>();
    test_array_roundtrip<int32_t>();
    test_array_roundtrip<int64_t>();
    test_array_roundtrip<uint8_t>();
    test_array_roundtrip<uint16_t>();
    test_array_roundtrip<uint32_t>();
    test_array_roundtrip<uint64_t>();
    test_array_roundtrip<float>();
    test_array_roundtrip<double>();
    test_array_roundtrip<std::complex<float>>();
    test_array_roundtrip<std::complex<double>>();
}

TEST_CASE("tensordm dataset dataset roundtrip")
{
    using namespace WireCell::PointCloud;
    using namespace WireCell::Aux;

    Array orig_one({1  ,2  ,3  });
    Array orig_two({1.1,2.2,3.3});

    Dataset d({
            {"one", orig_one},
            {"two", orig_two},
        });

    {
        auto one = d.get("one");
        CHECK(one->size_major() == 3);
        CHECK(one->is_type<int>());
        auto two = d.get("two");
        CHECK(two->size_major() == 3);
        CHECK(two->is_type<double>());

        for (size_t ind = 0; ind<3; ++ind) {
            CHECK(one->element<int>(ind) == orig_one.element<int>(ind));
            CHECK(two->element<double>(ind) == orig_two.element<double>(ind));
        }
    }

    const std::string datapath = "test";

    // We do not expect the round trip to step on our dataset metadata.
    const std::string not_an_ident_number = "this isn't an ident";
    d.metadata()["ident"] = not_an_ident_number;

    // to tensor

    auto itenvec = TensorDM::as_tensors(d, datapath);
    CHECK(itenvec.size() > 0);
    auto itsp = TensorDM::as_tensorset(itenvec, 42);
    CHECK(itsp->ident() == 42);
    auto itsmd = itenvec[0]->metadata();

    auto itv = itsp->tensors();
    {
        CHECK(itv->size() == 2+1);
        for (size_t ind=0; ind<3; ++ind) {
            auto it = itv->at(ind);
            auto md = it->metadata();
            if (!ind) {         // the aggregator
                CHECK(md["datatype"].asString() == "pcdataset");
            }
            else {              // an array
                CHECK(md["datatype"].asString() == "pcarray");
            }
        }
    }
    {
        auto one = itv->at(1);
        auto two = itv->at(2);

        for (size_t ind=0; ind<3; ++ind) {
            CHECK(orig_one.element<int>(ind) == ((int*)one->data())[ind]);
            CHECK(orig_two.element<double>(ind) == ((double*)two->data())[ind]);
        }
    }
    

    // back to dataset

    bool share = false;
    Dataset d2 = TensorDM::as_dataset(itsp, datapath, share);

    CHECK(d2.size_major() == 3);

    auto dmd = d2.metadata();
    CHECK(dmd["ident"].asString() == not_an_ident_number);

    auto keys = d2.keys();
    CHECK(keys.size() == 2);
    CHECK(std::find(keys.begin(), keys.end(), "one") != keys.end());
    CHECK(std::find(keys.begin(), keys.end(), "two") != keys.end());
    
    auto sel = d2.selection({"one", "two"});
    auto one = sel[0];
    auto two = sel[1];
    REQUIRE(one);
    REQUIRE(two);

    CHECK(one->size_major() == 3);
    CHECK(one->is_type<int>());
    CHECK(one->dtype() == "i4");
    auto one_bytes = one->bytes();
    CHECK(one_bytes.size() == 3*sizeof(int));
    // CHECK(1 == ((int*)one_bytes.data())[0]);

    CHECK(two->size_major() == 3);
    CHECK(two->is_type<double>());

    CHECK(one->metadata()["name"].isNull());

    for (size_t ind = 0; ind<3; ++ind) {
        CHECK(one->element<int>(ind) == orig_one.element<int>(ind));
        CHECK(two->element<double>(ind) == orig_two.element<double>(ind));
    }

    return;
}

 
