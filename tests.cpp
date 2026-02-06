
#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "providers/resource_provider.h"
#include "voxel/scene.h"
#include "voxel/world.h"
#include "voxel/raycast.h"
#include "voxel/simulation.h"
#include "ui/stage.h"
#include "datahub/datahub.h"

std::string testDesc0 = "v0 : integer = 17\r\nv1 : number = 678.3400\r\nv2 : bool = true\r\nv3 : string = \"ttt\"\r\n";
std::string testDesc1 = R"(
v0 : vector2i = 10 20
v1 : vector3i = 10 20 30
v2 : vector4i = 10 20 30 40
v3 : vector2f = 0.1100 0.2200
v4 : vector3f = 0.1100 0.2200 0.3300
v5 : vector4f = 0.1100 0.2200 0.3300 0.4400
)";
std::string testDesc2 = R"(
parameters {
    name : string = "test test test "
    offset : vector3i = 1 2 3
}
)";
std::string testDesc3 = R"(
parameters {
    animations {
        attack : vector3i = 6 17 20
        idle : vector3i = 0 5 10
    }
    offset : vector3i = 1 2 3
}
)";
std::string testDesc4 = R"(
strings : string[2] = "aaaa"  "bbbbbbbbbbbbbbb"
offset : vector2i[3] = 10 11  20 21  30 31
)";
std::string testDesc5 = R"(
object {
    name : string = "111"
}
object {
    name : string = "222"
}
object {
    name : string = "333"
}
)";
std::string testDesc6 = R"(test : string[2] = "qwerty"  "grimm")";


foundation::PlatformInterfacePtr platform;

void testUtilCallback() {

}

void testUtilStrstream() {
    
}

void testUtilDescription() {
    {
        util::Description d0 = util::Description::parse((const std::uint8_t *)testDesc0.data(), testDesc0.length());
        const auto *v0 = d0.getInteger("v0");
        assert(v0);
        assert(*v0 == 17);
        const auto *v1 = d0.getInteger("v01");
        const auto *v2 = d0.getVector2f("v0");
        assert(v1 == nullptr);
        assert(v2 == nullptr);
        const auto *v3 = d0.getNumber("v1");
        assert(v3);
        assert(std::fabs(*v3 - 678.34) < 0.0001f);
        const auto *v4 = d0.getBool("v2");
        assert(v4);
        assert(*v4);
        const auto *v5 = d0.getString("v3");
        assert(v5);
        assert(*v5 == "ttt");
        std::string serialized = util::Description::serialize(d0);
        assert(serialized == testDesc0);
    }
    {
        util::Description d0 = util::Description::parse((const std::uint8_t *)testDesc1.data(), testDesc1.length());
        const auto *v0 = d0.getVector2i("v0");
        assert(v0);
        assert(v0->x == 10 && v0->y == 20);
        const auto *v1 = d0.getVector3i("v1");
        assert(v1);
        assert(v1->x == 10 && v1->y == 20 && v1->z == 30);
        const auto *v2 = d0.getVector4i("v2");
        assert(v2);
        assert(v2->x == 10 && v2->y == 20 && v2->z == 30 && v2->w == 40);
        const auto *v3 = d0.getVector2f("v3");
        assert(v3);
        assert(std::fabs(v3->x - 0.11) < 0.0001f && std::fabs(v3->y - 0.22) < 0.0001f);
        const auto *v4 = d0.getVector3f("v4");
        assert(v4);
        assert(std::fabs(v4->x - 0.11) < 0.0001f && std::fabs(v4->y - 0.22) < 0.0001f && std::fabs(v4->z - 0.33) < 0.0001f);
        const auto *v5 = d0.getVector4f("v5");
        assert(v5);
        assert(std::fabs(v5->x - 0.11) < 0.0001f && std::fabs(v5->y - 0.22) < 0.0001f && std::fabs(v5->z - 0.33) < 0.0001f && std::fabs(v5->w - 0.44) < 0.0001f);
        std::string serialized = util::Description::serialize(d0);
        std::string nominal = testDesc1;
        nominal.erase(std::remove_if(nominal.begin(), nominal.end(), [](char c) { return c == '\r' || c == '\n'; }), nominal.end());
        serialized.erase(std::remove_if(serialized.begin(), serialized.end(), [](char c) { return c == '\r' || c == '\n'; }), serialized.end());
        assert(serialized == nominal);
    }
    {
        util::Description d0 = util::Description::parse((const std::uint8_t *)testDesc2.data(), testDesc2.length());
        const auto *s0 = d0.getDescription("parameters");
        assert(s0);
        const auto *v1 = s0->getVector3i("offset");
        assert(v1);
        assert(v1->x == 1);
        assert(v1->y == 2);
        assert(v1->z == 3);
        const auto *v2 = s0->getString("name");
        assert(v2);
        assert(*v2 == "test test test ");
        std::string nominal = testDesc2;
        std::string serialized = util::Description::serialize(d0);
        nominal.erase(std::remove_if(nominal.begin(), nominal.end(), [](char c) { return c == '\r' || c == '\n'; }), nominal.end());
        serialized.erase(std::remove_if(serialized.begin(), serialized.end(), [](char c) { return c == '\r' || c == '\n'; }), serialized.end());
        assert(serialized == nominal);
    }
    {
        util::Description d0 = util::Description::parse((const std::uint8_t *)testDesc3.data(), testDesc3.length());
        const auto *s0 = d0.getDescription("parameters");
        assert(s0);
        const auto *s1 = s0->getDescription("animations");
        assert(s1);
        const auto *v1 = s0->getVector3i("offset");
        assert(v1);
        assert(v1->z == 3);
        const auto m0 = s1->getVector3is();
        assert(m0.size() == 2);
        assert(m0.at("idle").y == 5);
        assert(m0.at("attack").y == 17);
        std::string nominal = testDesc3;
        std::string serialized = util::Description::serialize(d0);
        nominal.erase(std::remove_if(nominal.begin(), nominal.end(), [](char c) { return c == '\r' || c == '\n'; }), nominal.end());
        serialized.erase(std::remove_if(serialized.begin(), serialized.end(), [](char c) { return c == '\r' || c == '\n'; }), serialized.end());
        assert(serialized == nominal);
    }
    {
        util::Description d0 = util::Description::parse((const std::uint8_t *)testDesc4.data(), testDesc4.length());
        const auto m0 = d0.getVector2is("offset");
        assert(m0.size() == 3);
        assert(m0[0].x == 10 && m0[0].y == 11);
        assert(m0[1].x == 20 && m0[1].y == 21);
        assert(m0[2].x == 30 && m0[2].y == 31);
        const auto m1 = d0.getStrings("strings");
        assert(m1[0] == "aaaa");
        assert(m1[1] == "bbbbbbbbbbbbbbb");
        assert(m1.size());
    }
    {
        util::Description d0 = util::Description::parse((const std::uint8_t *)testDesc4.data(), testDesc4.length());
        std::string serialized = util::Description::serialize(d0);
        printf("");
    }

}

void testUtil() {
    testUtilCallback();
    testUtilStrstream();
    testUtilDescription();
}

extern "C" void initialize() {
    testUtil();
    
    platform = foundation::PlatformInterface::instance();
    platform->setLoop([](float dtSec) {
        platform->exit();
    });
}

extern "C" void deinitialize() {

}
