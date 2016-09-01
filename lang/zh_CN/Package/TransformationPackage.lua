--[[********************************************************************
	Copyright (c) 2013-2015 Mogara

  This file is part of QSanguosha-Hegemony.

  This game is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 3.0
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  See the LICENSE file for more details.

  Mogara
*********************************************************************]]

-- translation for Standard General Package

return {
	["transformation"] = "君临天下·变",
	["transformation_equip"] = "君临天下·变",

	--魏
	["#xunyou"] = "曹魏的谋主",
	["xunyou"] = "荀攸",
	["designer:xunyou"] = "淬毒",
	["illustrator:xunyou"] = "魔鬼鱼",
	["qice"] = "奇策",
	[":qice"] = "出牌阶段限一次，你可以将所有手牌当一张目标数不大于X的非延时类锦囊牌使用（X为你的手牌数），当此牌结算后，你可以变更副将的武将牌。",
	["zhiyu"] = "智愚",
	[":zhiyu"] = "每当你受到伤害后，你可以摸一张牌：若如此做，你展示所有手牌。若你的手牌均为同一颜色，伤害来源弃置一张手牌。",
	["$qice1"] = "亲力为国，算无遗策。",
	["$qice2"] = "奇策在此，谁与争锋。",
	["$zhiyu1"] = "大勇若怯，大智如愚。",
	["$zhiyu2"] = "愚者既出，智者何存。",
	["~xunyou"] = "主公……臣下先行告退……",

	["bianhuanghou"] = "卞夫人",
	["#bianhuanghou"] = "奕世之雍裔",
	["wanwei"] = "挽危",
	[":wanwei"] = "当你成为【过河拆桥】的目标时，你可以弃置一张牌，你取消自己；当你成为【顺手牵羊】的目标时，你可以交给对方一张牌，你取消自己。",
	--[":wanwei"] = "当你因被其他角色获得或弃置而失去牌时，你可以改为自己选择失去的牌。",
	["@wanwei1"] = "弃置一张牌，然后取消 %src 对你使用的【过河拆桥】",
	["@wanwei2"] = "交给 %src 一张牌，然后取消其对你使用的【顺手牵羊】",
	["yuejian"] = "约俭",
	[":yuejian"] = "与你势力相同角色的弃牌阶段开始时，若其本回合未使牌指定过除该角色外的其他角色，你可令其本回合手牌上限等于其体力上限。",

	-- 群
	["liguo"] = "李傕＆郭汜",
	["#liguo"] = "飞熊狂豹",
	["&liguo"] = "李傕郭汜",
	["xichou"] = "隙仇",
	[":xichou"] = "锁定技，当你第一次明置此武将牌时，你加2点体力上限并回复2点体力；当你于出牌阶段使用与本阶段你使用的上一张牌不同颜色的牌时，若没有角色处于濒死状态，你失去1点体力。",
	
	["#zuoci"] = "谜之仙人",
	["zuoci"] = "左慈",
	["illustrator:zuoci"] = "废柴男",
	["huashen"] = "化身",
	["huashencard"] = "化身牌",
	[":huashen"] = "准备阶段开始时，你可以观看剩余武将牌堆中的五张牌，亮出其中两张相同势力的武将牌置于你的武将牌旁，称为“化身”（若已有“化身”则将原来的“化身”置入剩余武将牌堆），你获得“化身”的技能，暗置你的另一张武将牌；若此武将牌处于明置状态，你不能明置另一张武将牌。",
	["xinsheng"] = "新生",
	[":xinsheng"] = "当你受到伤害后，你可以从剩余武将牌中连续亮出武将牌，直到亮出一张与“化身”相同势力的武将牌为止，然后你可以将之与一张“化身”替换。",
	["#dropHuashenDetail"] = "%from 丢弃了“化身牌” %arg",
	["#GetHuashenDetail"] = "%from 获得了“化身牌” %arg",
	["cv:zuoci"] = "东方胤弘，眠眠",
	["$huashen1"] = "藏形变身，自在吾心。",
	["$huashen2"] = "遁形幻千，随意所欲。",
	["$xinsheng1"] = "吐故纳新，师法天地。",
	["$xinsheng2"] = "灵根不灭，连绵不绝。",
	["$huashen3"] = "藏形变身，自在吾心。(女声)",
	["$huashen4"] = "遁形幻千，随意所欲。(女声)",
	["$xinsheng3"] = "吐故纳新，师法天地。(女声)",
	["$xinsheng4"] = "灵根不灭，连绵不绝。(女声)",
	["~zuoci"] = "释知遗形，神灭形消。",

	-- 蜀
	["shamoke"] = "沙摩柯",
	["#shamoke"] = "",
	["jili"] = "蒺藜",
	[":jili"] = "每当你于出牌阶段内使用第X张牌时，你可以摸X张牌；你可以额外使用X张【杀】（X为你装备区里武器牌的攻击范围）。",

	["masu"] = "马谡",
	["#masu"] = "兵法在胸",
	["sanyao"] = "散谣",
	[":sanyao"] = "出牌阶段限一次，你可以弃置一张牌，然后对体力最多的一名角色造成1点伤害。",
	["zhiman"] = "制蛮",
	[":zhiman"] = "当你对其他角色造成伤害时，你可以防止此伤害，然后你获得其装备区或判定区的一张牌。若该角色与你势力相同，则其可以明置所有武将牌，并变更其副将的武将牌。",
	["#Zhiman"] = "%from 防止了对 %to 的伤害",
	["$sanyao1"] = "散谣惑敌，不攻自破",
	["$sanyao2"] = "三人成虎事多有",
	["$zhiman1"] = "丞相多虑，且看我的",
	["$zhiman2"] = "兵法谙熟于心，取胜千里之外",
	["~masu"] = "败军之罪，万死难赎" ,

	-- 吴
	["#lingtong"] = "旋略勇进",
	["lingtong"] = "凌统",
	["liefeng"] = "烈风",
	[":liefeng"] = "每当你失去装备区里的牌时，可以弃置一名其他角色一张牌。",
	["liefeng-invoke"] = "你可以发动“烈风”<br/> <b>操作提示</b>: 选择一名有牌的其他角色→点击确定<br/>",
	["xuanlue"] = "旋略",
	[":xuanlue"] = "限定技，出牌阶段，你可以获得场上至多三张装备牌，然后将这些牌置入至多三名角色装备区内",
	["@xuanlue"] = "选择并获得场上至多三张装备牌",
	["@lue"] = "旋略",
	["xuanlue-get"] = "选择一名有装备牌的其他角色",
	["@xuanlue-distribute"] = "旋略：你可将至多 %arg 张牌进行分配",
	["~xuanlue_equip"] = "选择合理的装备牌牌和一名其他角色→点击确定",
	["$liefeng1"] = "伤敌于千里之外！",
	["$liefeng2"] = "索命于须臾之间！",
	["~lingtong"] = "大丈夫…不惧死亡……",

	["lvfan"] = "吕范",
	["#lvfan"] = "忠篆亮直",
	["diaodu"] = "调度",
	["diaoduequip"] = "调度",
	[":diaodu"] = "出牌阶段限一次，你可令所有与你相同势力的角色依次选择一项：使用手牌中的一张装备牌，或将装备区里的一张牌移动至另一名与你相同势力的角色的装备区。",
	["@Diaodu-distribute"] = "使用一张装备牌，或将装备区一张牌移动至另一名同势力角色的装备区",
	["~diaodu_equip"] = "选择一张手牌中的装备牌，或选择装备区的一张牌和一名与你同势力的其他角色",
	["$DiaoduEquip"] = "%from 被装备了 %card",
	["diancai"] = "典财",
	[":diancai"] = "其他角色的出牌阶段结束时，若你本阶段失去了X张或更多的牌（X为你已损失的体力值），你可以将手牌补至体力上限，然后你可以变更副将的武将牌。",


	["#lord_sunquan"] = "东吴大帝",
	["lord_sunquan"] = "孙权-君",
	["&lord_sunquan"] = "孙权",
	["illustrator:lord_sunquan"] = "",
	["jiahe"] = "嘉禾",
	[":jiahe"] = "君主技，若此武将牌处于明置状态，你便拥有“缘江烽火图”。",
	["lianzi"] = "敛资",
	[":lianzi"] = "出牌阶段限一次，你可以弃置一张手牌，然后观看牌堆顶的X张牌（X为吴势力角色装备牌里的牌数量之和），展示并获得其中任意张与你弃置的牌相同类别的牌。",
	["jubao"] = "聚宝",
	[":jubao"] = "锁定技，你装备区里的宝物牌不能被其他角色获得；结束阶段开始时，你获得装备区里有【定澜夜明珠】的角色的一张牌，然后摸一张牌。",
	["@lianzi"] = "选择并获得与你弃置的牌相同类别的牌",
	["lianzi#up"] = "牌堆",
	["lianzi#down"] = "获得",
	["flamemap"] = "缘江烽火图",
	["flame_map"] = "缘江烽火图",
	[":flamemap"] = "【缘江】出牌阶段限一次，与你势力相同的角色可以将一张装备牌置于“缘江烽火图”上。【烽火】锁定技，当你受到伤害后，你将缘江烽火图上的一张牌置入弃牌堆。【宏图】“吴”势力角色依据“缘江烽火图”上的牌数视为拥有以下技能：1、英姿；2、好施；3、攻心；4、谦逊。",
	["&flamemap"] = "【缘江】出牌阶段限一次，你可以将一张装备牌置于“缘江烽火图”上。【宏图】“吴”势力角色依据“缘江烽火图”上的牌数视为拥有以下技能：1、英姿；2、好施；3、攻心；4、谦逊。",
	["yingziextra"] = "英姿",
	["@flamemap"] = "请弃置一张“缘江烽火图”上的牌",
	["~flamemap"] = "选择一张“缘江烽火图”上的牌：点击确定",

	["Luminouspearl"] = "定澜夜明珠",
	[":Luminouspearl"] = "装备牌·宝物\n\n技能：锁定技，你视为拥有技能“制衡”若你已拥有“制衡”，则改为取消弃置牌数的限制。",
	["luminouspearl"] = "制衡",

	["gongxin"] = "攻心",
	[":gongxin"] = "出牌阶段限一次，你可以观看一名其他角色的手牌，然后选择其中一张红桃牌并选择一项：弃置之，或将之置于牌堆顶。",
	["#gongxin"] = "攻心 %log",
	["gongxin:discard"] = "弃置",
	["gongxin:put"] = "置于牌堆顶",
	["yingzi"] = "英姿",

	["transform"] = "变更副将",
	["GameRule:ShowGeneral"] = "选择需要明置的武将",
}

